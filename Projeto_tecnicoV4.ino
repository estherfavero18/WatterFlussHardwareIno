/* Bibliotecas */ 
#include <WiFi.h> /* Header para uso das funcionalidades de wi-fi do ESP32 */
#include <PubSubClient.h>  /*  Header para uso da biblioteca PubSubClient */

#define MSG_BUFFER_SIZE (50)
char msgDistancia[MSG_BUFFER_SIZE]; 
char msgVEntrada[MSG_BUFFER_SIZE];
char msgVSaida[MSG_BUFFER_SIZE];  

/* Tópico MQTT para recepção de informações do broker MQTT para ESP32 */
#define TOPICO_SUBSCRIBE "Recebe_informacao"   

/* Tópicos MQTT para envio de informações do ESP32 para broker MQTT */
#define TOPICO_PUBLISH_DISTANCIA   "Distancia" 
#define TOPICO_PUBLISH_VAZAO_ENTRADA   "VazaoEntrada"  
#define TOPICO_PUBLISH_VAZAO_SAIDA   "VazaoSaida"  
#define ID_MQTT  "WatterFluss_MQTT" /* id mqtt (para identificação de sessão) */    

/*  Variáveis e constantes globais */
const char* SSID = "WILLIAM 2G"; /* SSID / nome da rede WI-FI que deseja se conectar */
const char* PASSWORD = "31997406098";  /*  Senha da rede WI-FI que deseja se conectar */
const char* BROKER_MQTT = "watterflussmqtt.cloud.shiftr.io"; /* URL do broker MQTT Utilizado */
const char* USERNAME_MQTT = "watterflussmqtt";
const char* PASSWORD_MQTT = "isTz40tgY7yWkMQa";
int BROKER_PORT = 1883;/* Porta do Broker MQTT */
unsigned long currentTime;
unsigned long lastTime;
double fluxoEntrada;
unsigned long  flow_frequencyEntrada; 
double fluxoSaida;
unsigned long  flow_frequencySaida; 
float distancia;

/*Pinos*/
int flowPinEntrada = 19;    //Este é o pino de entrada do sensor de Vazão de Entrada
int flowPinSaida = 18;    //Este é o pino de entrada do sensor de Vazão de Saida
int pinTrigger = 13;  //Este é o pino de entrada do pulso
int pinEcho = 12;  //Este é o pino de Saida do pulso
 
/* Variáveis e objetos globais */
WiFiClient espClient;
PubSubClient MQTT(espClient);
  
/*Funções */
void init_serial(void);//Inicia 
void init_wifi(void);
void init_mqtt(void);
void reconnect_wifi(void); 
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void verifica_conexoes_wifi_mqtt(void);
float leituraSimples(); //Realiza a leitura unitaria de Distancia
float calcularDistancia(); //Realiza o calculo médio das distancias unitarias
void sonarBegin(byte trig ,byte echo);
 
/* 
 *  Implementações das funções
 */
 
void ICACHE_RAM_ATTR flowEntrada () { // Função Interrupção
   flow_frequencyEntrada++; //Quando essa função é chamada, soma-se 1 a variável 
}
void ICACHE_RAM_ATTR flowSaida () { // Função Interrupção
   flow_frequencySaida++; //Quando essa função é chamada, soma-se 1 a variável 
}
void setup() 
{
    init_wifi();
    init_mqtt();
    pinMode(flowPinEntrada, INPUT);
    pinMode(flowPinSaida, INPUT);
    init_serial();
    sonarBegin(pinTrigger,pinEcho);
    attachInterrupt(digitalPinToInterrupt(flowPinEntrada), flowEntrada, RISING); // Setup Interrupt
    attachInterrupt(digitalPinToInterrupt(flowPinSaida), flowSaida, RISING); // Setup Interrupt
    currentTime = millis();
   lastTime = currentTime;
}
  
/* Função: inicializa comunicação serial com baudrate 115200 (para fins de monitorar no terminal serial o que está acontecendo.*/
void init_serial() 
{
    Serial.begin(115200);
}
 
/* Função: inicializa e conecta-se na rede WI-FI desejada*/
void init_wifi(void) 
{
    delay(10);
    Serial.println("------Conexao WI-FI------");
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID);
    Serial.println("Aguarde");
    reconnect_wifi();
}
  
/* Função: inicializa parâmetros de conexão MQTT(endereço do broker, porta e seta função de callback)*/
void init_mqtt(void) 
{
    /* informa a qual broker e porta deve ser conectado */
    MQTT.setServer(BROKER_MQTT, BROKER_PORT); 
    /* atribui função de callback (função chamada quando qualquer informação do 
    tópico subescrito chega) */
    MQTT.setCallback(mqtt_callback);            
}
  
/* Função: função de callback 
 *          esta função é chamada toda vez que uma informação de 
 *          um dos tópicos subescritos chega)
 * */
void mqtt_callback(char* topic, byte* payload, unsigned int length) 
{
    String msg;
 
    //obtem a string do payload recebido
    for(int i = 0; i < length; i++) 
    {
       char c = (char)payload[i];
       msg += c;
    }
    Serial.print("[MQTT] Mensagem recebida: ");
    Serial.println(msg);     
}
  
/* Função: reconecta-se ao broker MQTT (caso ainda não esteja conectado ou em caso de a conexão cair)
 *          em caso de sucesso na conexão ou reconexão, o subscribe dos tópicos é refeito.
 */
void reconnect_mqtt(void) 
{
    while (!MQTT.connected()) 
    {
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(ID_MQTT,USERNAME_MQTT,PASSWORD_MQTT)) 
        {
            Serial.println("Conectado com sucesso ao broker MQTT!");
            MQTT.subscribe(TOPICO_SUBSCRIBE); 
        } 
        else
        {
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Havera nova tentatica de conexao em 2s");
            delay(2000);
        }
    }
}
  
/* Função: reconecta-se ao WiFi*/
void reconnect_wifi() 
{
    /* se já está conectado a rede WI-FI, nada é feito. 
       Caso contrário, são efetuadas tentativas de conexão */
    if (WiFi.status() == WL_CONNECTED)
        return;
         
    WiFi.begin(SSID, PASSWORD);
     
    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(100);
        Serial.print(".");
    }
   
    Serial.println();
    Serial.print("Conectado com sucesso na rede ");
    Serial.print(SSID);
    Serial.println("IP obtido: ");
    Serial.println(WiFi.localIP());
}
 
/* Função: verifica o estado das conexões WiFI e ao broker MQTT. 
 *         Em caso de desconexão (qualquer uma das duas), a conexão
 *         é refeita.
 */
void verifica_conexoes_wifi_mqtt(void)
{
    /* se não há conexão com o WiFI, a conexão é refeita */
    reconnect_wifi(); 
    /* se não há conexão com o Broker, a conexão é refeita */
    if (!MQTT.connected()) 
        reconnect_mqtt(); 
} 
 
/* programa principal */
void loop() 
{  
    verifica_conexoes_wifi_mqtt(); /* garante funcionamento das conexões WiFi e ao broker MQTT */
    currentTime = millis();
    interrupts();  //sei(); // Enable interrupts 
    if(currentTime >= (lastTime + 1000)){
      lastTime = currentTime;
      fluxoEntrada = (flow_frequencyEntrada * 2.25); //Conta os pulsos no último segundo e multiplica por 2,25mL, que é a vazão de cada pulso
      flow_frequencyEntrada = 0;
      fluxoSaida = (flow_frequencySaida * 2.25); //Conta os pulsos no último segundo e multiplica por 2,25mL, que é a vazão de cada pulso
      flow_frequencySaida = 0;
  }
    distancia = calcularDistancia();
    sprintf(msgDistancia,"%f",distancia);
    sprintf(msgVEntrada,"%f",fluxoEntrada); 
    sprintf(msgVSaida,"%f",fluxoSaida);
    Serial.print("Distância: ");
    Serial.print(distancia); 
    Serial.print("   Vazão de Entrada: ");
    Serial.print(fluxoEntrada);
    Serial.print("   Vazão de saida: ");
    Serial.println(fluxoSaida);

    /* Envia frase ao broker MQTT */
    MQTT.publish(TOPICO_PUBLISH_VAZAO_ENTRADA,msgVEntrada);
    MQTT.publish(TOPICO_PUBLISH_VAZAO_SAIDA, msgVSaida);
    MQTT.publish(TOPICO_PUBLISH_DISTANCIA, msgDistancia);
    /* keep-alive da comunicação com broker MQTT */    
    MQTT.loop();
    /* Aguarda 1 segundo para próximo envio */
    delay(1000); 
}
void sonarBegin(byte trig ,byte echo) {
  #define divisor 58.0
  #define intervaloMedida 5 // MÁXIMO 35 mS PARA ATÉ 6,0M MIN 5mS PARA ATÉ 0,8M
  #define qtdMedidas 20 // QUANTIDADE DE MEDIDAS QUE SERÃO FEITAS
  pinMode(trig, OUTPUT); 
  pinMode(echo, INPUT);
  digitalWrite(trig, LOW); // DESLIGA O TRIGGER E ESPERA 500 uS
  delayMicroseconds(500);

}
float calcularDistancia() {
  float leituraSum = 0;
  float resultado = 0;
  for (int index = 0; index < qtdMedidas; index++) {
    delay(intervaloMedida);
    leituraSum += leituraSimples();
  }
  resultado = (float) leituraSum / qtdMedidas;
  resultado = resultado + 2.2;
  return resultado;
}

float leituraSimples() {
  long duracao = 0; 
  float resultado = 0;
  digitalWrite(pinTrigger, HIGH); 
  delayMicroseconds(10); 
  digitalWrite(pinTrigger, LOW);
  duracao = pulseIn(pinEcho, HIGH);
  resultado = ((float) duracao / divisor);
  return resultado;
}
