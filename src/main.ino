#include "esp_camera.h"
#include "esp_http_server.h"
#include "WiFi.h"

#define CAMERA_MODEL_AI_THINKER

// Camera pins for AI-THINKER
#if defined(CAMERA_MODEL_AI_THINKER)
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
  #define FLASH_LED_PIN      4
#endif

// Configuration
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASS "YOUR_WIFI_PASSWORD"
#define PIR_SENSOR        13  // GPIO13
#define IR_BEAM_SENSOR    14  // GPIO14
#define SECOND_IR_PIN      2  // GPIO2 (NEW SECONDARY IR)
#define OUTPUT_PIN_1      12  // GPIO12
#define OUTPUT_PIN_2      15  // GPIO15

// Optimized settings
#define CHECK_INTERVAL     50  // Faster sensor checks
#define OUTPUT_TIMEOUT    3000 // Output duration
#define HTML_REFRESH_INTERVAL 1 
#define FLASH_DELAY       50   
#define FRAME_SIZE        FRAMESIZE_VGA 
#define JPEG_QUALITY      8    

// Global Variables
camera_fb_t *latest_frame = NULL;
unsigned long last_trigger = 0;
bool outputs_active = false;
httpd_handle_t server = NULL;

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(5000);
  
  // Configure pins
  pinMode(PIR_SENSOR, INPUT);
  pinMode(IR_BEAM_SENSOR, INPUT);
  pinMode(SECOND_IR_PIN, INPUT_PULLUP);  // NEW PIN WITH PULLUP
  pinMode(OUTPUT_PIN_1, OUTPUT);
  pinMode(OUTPUT_PIN_2, OUTPUT);
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);
  digitalWrite(OUTPUT_PIN_1, LOW);
  digitalWrite(OUTPUT_PIN_2, LOW);

  // Initialize camera
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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAME_SIZE;
  config.jpeg_quality = JPEG_QUALITY;
  config.fb_count = 1;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed!");
    while (1) delay(1000);
  }

  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("\nConnecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n\n=================================");
  Serial.print("CAMERA SERVER IP: ");
  Serial.println(WiFi.localIP());
  Serial.println("=================================\n");

  // Start server
  startCameraServer();
  delay(2000);
}

void loop() {
  static unsigned long last_check = 0;
  
  if (millis() - last_check > CHECK_INTERVAL) {
    check_sensors();
    last_check = millis();
  }

  if (outputs_active && (millis() - last_trigger > OUTPUT_TIMEOUT)) {
    digitalWrite(OUTPUT_PIN_1, LOW);
    digitalWrite(OUTPUT_PIN_2, LOW);
    outputs_active = false;
  }
  
  delay(10);
}

void check_sensors() {
  // First check secondary IR (GPIO2)
  if(digitalRead(SECOND_IR_PIN) == LOW) {
    if(outputs_active) {
      // Immediately disable outputs if they were active
      digitalWrite(OUTPUT_PIN_1, LOW);
      digitalWrite(OUTPUT_PIN_2, LOW);
      outputs_active = false;
    }
    return; // Block all captures
  }

  // Then check primary sensors
  bool pir = digitalRead(PIR_SENSOR);
  bool beam = digitalRead(IR_BEAM_SENSOR);

  if (pir && !beam) {
    if (!outputs_active) {
      trigger_capture();
    }
    last_trigger = millis();
  }
}

void trigger_capture() {
  digitalWrite(OUTPUT_PIN_1, HIGH);
  digitalWrite(OUTPUT_PIN_2, HIGH);
  outputs_active = true;

  unsigned long capture_start = millis();
  digitalWrite(FLASH_LED_PIN, HIGH);
  
  camera_fb_t *fb = esp_camera_fb_get();
  
  if (millis() - capture_start >= FLASH_DELAY) {
    digitalWrite(FLASH_LED_PIN, LOW);
  }

  if (fb) {
    if (latest_frame) {
      esp_camera_fb_return(latest_frame);
    }
    latest_frame = fb;
    Serial.printf("New image captured in %lums\n", millis() - capture_start);
  }
}

void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;

  if (httpd_start(&server, &config) == ESP_OK) {
    httpd_uri_t index_uri = {
      .uri = "/",
      .method = HTTP_GET,
      .handler = [](httpd_req_t *req) {
        String html = "<html><head>"
                      "<meta http-equiv=\"refresh\" content=\"" + String(HTML_REFRESH_INTERVAL) + "\">"
                      "<title>ESP32-CAM Live</title></head><body>"
                      "<h1>Live Security Feed</h1>"
                      "<img src=\"/image?t=\" + Date.now() style=\"max-width: 100%;\">"
                      "</body></html>";
        httpd_resp_set_type(req, "text/html");
        return httpd_resp_send(req, html.c_str(), html.length());
      }
    };

    httpd_uri_t image_uri = {
      .uri = "/image",
      .method = HTTP_GET,
      .handler = [](httpd_req_t *req) {
        if (!latest_frame) return ESP_FAIL;
        
        httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store, must-revalidate");
        httpd_resp_set_hdr(req, "Pragma", "no-cache");
        httpd_resp_set_hdr(req, "Expires", "0");
        
        httpd_resp_set_type(req, "image/jpeg");
        return httpd_resp_send(req, (const char *)latest_frame->buf, latest_frame->len);
      }
    };

    httpd_register_uri_handler(server, &index_uri);
    httpd_register_uri_handler(server, &image_uri);
    Serial.println("HTTP server started");
  }
}
