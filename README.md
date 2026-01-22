# Overview

- This project has been created to wrap up some limitations from previous project like inrush conditions. The load aware control or current-based decision maker was tought to classify and measure dynamic consumption and encompasse large circuits that wasn't worked yet. Initially we think in control loads based in machine learning perceptions using decision tree, whereas the project didn't work so well, due to physical,economical limitations and timestime. However, this project was so useful because I learn how to detect noise, improve accurancies and make better decisions related to IoT projects. I learned snubber circuit, signal analyses, kind of SSRs and circuit impedance.

## Variables definition
- Foi pensando em criar uma variável de corrente da carga mínima de 0.15 ampér, em que servia para ignorar ruídos do ACS712-5B. Contudo, foi percebido que não foi necessário usar essa variável, pois em ajustes futuros foi tirado o ACS712-5B série com todo o circuito e foi conectado em série apenas com o motor. Assim, melhorando a acurácia da medição do sensor. 
Também foi retirado a variável chamada: overcome_limit de 0.7 ampér, pois não foi ligado simultaneamente as cargas em paralelo para evitar saturação de corrente. 
Também foi criado uma variável chamada inrush_limit que detectava a ativação do motor na partida e definir um limite de funcionamento do circuito com o motor ativo.
Também foi definido limite de estabilidade e tempo de estabilidade do circuito que vai ser usado na função de regime permanente.


# Architecture

## Physical circuit
- A implementação foi pensada como um melhoramento e teste do projeto anterior com ESP32, em que inicialmente tinha mais de 15 jumpers espalhados por toda a protoboard e 3 capacitores em paralelo, 1 LED vermelho e testei um resistor de 100 kohm na entrada do arranjo e saída do OUTPUT do ACS712-5B, mas isso causava pertubações nos sinais, devido a impedância dos fios. A imagem do circuito inicialmente pensado está ilustrado abaixo.

...

Portanto, implementei um circuito mais simples para reduzir os ruidos e aumentar a legibilidade do projeto que foi apenas um capacitor de 470 uF, um LED e 12 jumpers. Isso reduziu as oscilações e tornou mais simples e modular.
Além disso, foi implementando dois SSRs(relés de estado sólido) em série com as cargas de teste. Isso foi um aprendizado porque aprendi que há vários componentes eletrônicos que funcionam como optoacoplador e o SSR tem um optoacoplador internamente e serve para comutar os sinais DC e AC. Inicialmente o ACS712-5B alimentava os SSRs para depois ir para as cargas. 
Contudo foi percebido que esse arranjo físico não faz sentido, pois o ACS712-5B monitorava os ruidos do funcionamento da lâmpada junto com o motor e esse sensor não gera energia só observa a corrente. Assim, foi alterado o circuito como mostra a imagem a abaixo para o ACS712-5B ficar em série apenas com o motor. Assim, tornando as medições precisas e reduzindo ruidos. 

...

## State machine
- Foi pensando em uma máquina de estados para definir as ações do sistema para cada estado com inicialmente 6 estados, desde a inativação total do sistema até o estado de ativação de desativação de cada componente. Como a imagem abaixo mostra:

...

Contudo foi percebido que esse arranjo aumentava a complexidade do projeto e tornava muito confuso o entendimento dos estados e as possíveis ações. Assim, foi reduzido para 4 estados sendo: Idle, Lamp_On, Motor_Starting e Motor_On. Dessa forma, quando fosse ativo o cmd_off isso levava o sistema para o estado IDLE, que é a inatividade completa do sistema e evitando com que a lâmpada e o motor fossem ligados juntos, assim, evitando saturação e mal funcionamento dos componentes como mostra a imagem abaixo.

...

## State machine funcionalities
- O primeiro estado foi o IDLE, em que desativa todos as cargas e verifica o comando definido pelo usuário de ligar o motor ou a lâmpada. Caso o comando solicitasse a ativação da lâmpada. Isso iria para o estado Lamp_On que ativa a lâmpada, verifica se for mandado o comando de ativar o motor a lâmpada é desativada, liga o motor e começa a contar o tempo de inicialização para ser verificado o tempo de estabilização. Se foi mandado o comando de cmd_off o sistema é desligado por completo e volta ao IDLE.
Caso fosse solicitado a ativação do motor. Isso iria para o estado intermediário chamado Motor_Starting que verifica o inrush current com o inrush_limit, se o inrush for maior que o inrush_limit o sistema é desativado, caso contrário isso obedece o tempo de estabilização e o motor é ativado e dentro do estado Motor_On caso ativado a lâmpada o motor é desativado e a lâmpada é ativada passando para o estado Lamp_On. No final da FSM os comandos são limpados evitando uma acumulação de comandos e quebra da FSM.

# Observations

- Foi testado o projeto com 1 resistor de 100 kohm e 1 capacitor de 10 nanoF. Contudo, pela fórmula -> 1/ (2piRC) -> 159.15 Hz. Assim, ele atenuava as frequências baixas do sensor, mas junto atenuava os sinais enviados pela carga. Assim, causando um falso-positivo na medição, em que os sinais oscilava muito na janela de 200 ms que foi o tempo de amostragem para detecção de peak current. Além disso, antes tinha colocado como peak current 0.1 o que não faz sentido, não só porque os ruidos padrões do ACS712-5B já são em média 0.35, mas que nem toda corrente de funcionamento maior que 0.1 são pico de corrente.

- Se apenas ligando a lâmpada sozinha uma corrente maior que 1.1 já é um pico de corrente na partida.

- Além disso, devido as oscilações intermitentes do sinal foi testado na protoboard apenas com o capacitor de 10 nanoF, mas isso não mudou o comportamento caótico do sistema.
Assim, foi testado um arranjo de 3 capacitores em paralelo com cada sendo 220 microF dando 660 uF. Isso ajudou na contenção das oscilações e quando foi ligado o cooler em paralelo com a lâmpada (cargas) o pisca pisca da lâmpada parou e só ficou piscando o LED colocado em paralelo com os capacitores na protoboard.

- Também, foi visto que para load-aware control foi necessário de 2 relés de estado sólidos (SSRs) para ligar e desligar o cooler e lâmpada pelo machine learning usando decision tree. A árvore de decisão iria controlar os SSRs enviando sinais de ativação e desativação baseado na corrente e potência.

- Não foi necessário a função de Motor_On, pois isso é redundante e causa menor legibilidade ao código.

# Code
- Primeiramente são definidos as pinagens do sensor e dos SSRs. Também é medido os valores dos offsets baseados nas medições da entrada analógica e calculado o offset que é os ruídos das medições do sensor.
A função de leitura de corrente detecta o sinal analógico adc da entrada do pino 34 do ESP32, cria a tensão adc baseado nas referências e especificações do sensor. Também, inicializa o valor da corrente instantênea baseado nas referências. Logo adiante, é detectado o inrush current enviado pelo motor analisando a janela de 14700 micro segundos avaliando o valor absoluto da corrente instantênea com o pico de corrente.

...

Ademais, na função de atualização do root mean square é feito com uma mostra de frequência de 500 Hz usando a fórmula: raiz quadrada do quadrado da corrente instantânea pelo número de tentativas.
Além disso, a função de regime permanente atualiza o erro baseado no último valor de o root mean square da corrente e verifica se o erro estar numa janela de estabilidade condizente com o Stable_Thresh variable.
No final é feito a chamada de todas as funções do sistema.

# Conclusion
