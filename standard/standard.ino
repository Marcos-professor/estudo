#include <LittleFS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <ESP8266mDNS.h>

//OTA
const char* OTA_USER = "admin";
const char* OTA_PASS = "esp8266";

ESP8266WebServer server(80);

ESP8266HTTPUpdateServer httpUpdater;

//--------------------utilitarios--------------

//tipo de arquivos para upload
String getContentTypeUpload(String filename) {
  if (filename.endsWith(".html")) return "text/html";
  if (filename.endsWith(".css"))  return "text/css";
  if (filename.endsWith(".js"))   return "application/javascript";
  if (filename.endsWith(".jpg"))  return "image/jpeg";
  if (filename.endsWith(".png"))  return "image/png";
  if (filename.endsWith(".ico"))  return "image/x-icon";
  return "text/plain";
}

//----------------------pages---------------------
void handleFilesLittle(){
  File file = LittleFS.open("/files.html", "r");
  if(!file){
    server.send(404, "text/plain", "Arquivo file.html nao encontrado!");
    return;
  }

  server.streamFile(file, "text/html");
  file.close();
}


//-----------------------API------------------
void handleListarArquivos() {
  // Criar um documento JSON dinâmico
  // 1024 é o tamanho do buffer em bytes. Ajuste conforme a quantidade de arquivos.
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();

  Dir dir = LittleFS.openDir("/");
  while (dir.next()) {
    // Adiciona cada nome de arquivo ao array JSON
    array.add(dir.fileName());
  }

  // Serializa o JSON para uma String
  String output;
  serializeJson(doc, output);

  
  server.send(200, "application/json", output);
}

//subir arquivos para a memoria flash
void handleFileUpload() {
  HTTPUpload& upload = server.upload();
  static File fsUploadFile;

  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    // Garante que o arquivo comece com /
    if (!filename.startsWith("/")) filename = "/" + filename;
    
    // Abrir em "w" sobrescreve automaticamente se já existir
    fsUploadFile = LittleFS.open(filename, "w");
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile) fsUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) fsUploadFile.close();
  }

  //server.send(200, "text/plain", "Upload concluido!"); não pode ficar na função
}

void handleNotFound() {
  String path = server.uri();
  if (LittleFS.exists(path)) {
    File file = LittleFS.open(path, "r");
    server.streamFile(file, getContentTypeUpload(path)); // Usa sua função existente
    file.close();
  } else {
    server.send(404, "text/plain", "Arquivo nao encontrado");
  }
}

void setup() {
  Serial.begin(115200);
  LittleFS.begin();

  WiFi.mode(WIFI_STA);
  WiFi.begin("Ativa Tech_EXT","@Ativa053");

  Serial.println("conectando");

  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  Serial.println("Conectado. seu IP: ");
  Serial.print(WiFi.localIP());

  //inicialização do mDNS
  if(MDNS.begin("teste")){
    Serial.println("mDNS inicializado no http://teste.local");
  }else{
    Serial.println("Erro na inicialização do mDNS");
  }

  //---------------------rotas--------------------

  

  // Configurar OTA HTTP
  httpUpdater.setup(&server, "/update", "admin", "123456789");
  //                          ↑ Rota   ↑ Usuário  ↑ Senha 

  server.on("/", handleFilesLittle);
  server.onNotFound(handleNotFound);

  //----------------------API------------------------

  //listagem de arquivos flash
  server.on("/lista-little", HTTP_GET, handleListarArquivos);

  // Rota para processar o upload
  server.on("/uploadFiles", HTTP_POST, []() {
    server.send(200, "text/plain", "Upload concluido com sucesso!");
  }, handleFileUpload);

  //------------------inicialização----------------

  server.begin();

  //OTA http
  Serial.println("✅ OTA HTTP habilitado!");
  Serial.println("Acesse: http://" + WiFi.localIP().toString() + "/update");
}

void loop() {
  server.handleClient();
  MDNS.update();
}
