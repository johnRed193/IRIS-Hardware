#include <WiFi.h>
#include <HTTPClient.h>  
#include "esp_camera.h"

#define CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

const char* ssid = "sirny_palochke";
const char* password = "01906714";

const char* serverUrl = "http://iris.projects.chirag.sh/detect.phpn";

camera_fb_t* fb = NULL;

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
    //Инициализация камеры(тут возможно другие пины или конфиги, взял пример просто из инета)
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.frame_size = FRAMESIZE_UXGA;
    config.pixel_format = PIXFORMAT_JPEG; // for streaming
    //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 12;
    config.fb_count = 1;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }
}

void loop() {
    captureAndSendPhoto();
    delay(5000); //Тут в миллисекундах разница как часто отправлять фотки
}


void captureAndSendPhoto() {

    //Получаем указатель на информацию о снимке(тоже из инета) 
    fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        return;
    }
    //В отдельную функцию вынес именно отправку на сервер
    sendPhotoToServer(fb->buf, fb->len);
}


void sendPhotoToServer(uint8_t* imageData, size_t imageSize) {
    WiFiClient client;
    //Проверяем проходит ли коннект к серверу
    if (!client.connect(serverUrl, 80)) {
    Serial.println("Failed to establish connection to the server");
}
    String boundary = "--------------------------123456789012345678901234";
    String contentType = "multipart/form-data; boundary=" + boundary;

    String header = "--" + boundary + "\r\n";
    header += "Content-Disposition: form-data; name=\"photo\"; filename=\"photo.jpg\"\r\n";
    header += "Content-Type: image/jpeg\r\n\r\n";

    String footer = "\r\n--" + boundary + "--\r\n";

    size_t contentLength = imageSize + header.length() + footer.length();

    client.println("POST /detect_person HTTP/1.1");
    client.println("Host: http://iris.projects.chirag.sh");
    client.println("User-Agent: Mozilla/5.0");
    client.println("Connection: close");
    client.println("Content-Type: " + contentType);
    client.print("Content-Length: ");
    client.println(contentLength);
    client.println();
    client.print(header);
    client.write(imageData, imageSize);
    client.print(footer);
    
    unsigned long timeout = millis();
    while (client.available() == 0) {
        if (millis() - timeout > 5000) {
            Serial.println("Client Timeout !");
            client.stop();
            return;
        }
    }

    // Читаем ответ от сервера
    while (client.available()) {
        String line = client.readStringUntil('\r');
        Serial.print(line);
    }

    Serial.println();
    Serial.println("Closing connection");
    client.stop();

}
