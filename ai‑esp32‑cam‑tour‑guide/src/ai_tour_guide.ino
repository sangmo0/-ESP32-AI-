/*
 * AI 实景人文导游 —— ESP32-CAM + 多模态大模型
 * ------------------------------------------------------------
 * 硬件: ESP32-CAM (OV2640)
 * 流程: 串口触发拍照 -> Base64 编码 -> HTTPS 调用火山方舟多模态 API
 *       -> 大模型返回真人风格导游讲解文本 -> 串口输出
 * ------------------------------------------------------------
 * 注意: 上传前请先在下方「用户配置区」填入你自己的 WiFi / API Key / 模型 Endpoint
 */

#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "base64.h"

// ====================== 用户配置区 ======================
// WiFi 配置
const char* WIFI_SSID     = "your_wifi_ssid";
const char* WIFI_PASSWORD = "your_wifi_password";

// 火山方舟 API 配置
const char* DOUBAO_API_KEY = "your_ark_api_key";
const char* DOUBAO_HOST    = "ark.cn-beijing.volces.com";
const int   DOUBAO_PORT   = 443;
const char* MODEL_NAME      = "ep-xxxxxxxx";   // 替换为你的模型 endpoint id
// =======================================================

#define TAKE_PHOTO_TRIGGER 'p'

// ESP32-CAM (AI-Thinker) 标准引脚
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

WiFiClientSecure client;
bool camera_ok = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=====================================");
  Serial.println("AI 实景人文导游 完整版");
  Serial.println("=====================================");

  client.setInsecure();          // 演示用: 跳过证书校验; 生产环境建议安装 CA
  client.setTimeout(30000);

  Serial.println("正在初始化摄像头...");
  camera_ok = initCamera();
  connectWiFi();

  if (camera_ok && WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ 设备就绪! 串口输入 p + 回车 开始实景讲解");
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) { delay(1000); return; }
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == TAKE_PHOTO_TRIGGER && camera_ok) {
      Serial.println("\n📸 拍摄实景画面, 开始深度讲解...");
      takePhotoAndAskDoubao();
    }
  }
  delay(50);
}

// ---------------- 摄像头初始化 ----------------
bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer    = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk    = XCLK_GPIO_NUM;
  config.pin_pclk    = PCLK_GPIO_NUM;
  config.pin_vsync   = VSYNC_GPIO_NUM;
  config.pin_href    = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn   = PWDN_GPIO_NUM;
  config.pin_reset  = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size   = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count     = 2;
    config.grab_mode    = CAMERA_GRAB_LATEST;
    config.fb_location  = CAMERA_FB_IN_PSRAM;
  } else {
    config.frame_size   = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count     = 1;
    config.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location  = CAMERA_FB_IN_DRAM;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("❌ 摄像头初始化失败 0x%x\n", err);
    return false;
  }

  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_QVGA);   // 控制体积, 兼顾识别率
  s->set_vflip(s, 0);
  s->set_hmirror(s, 0);

  Serial.println("✅ 摄像头启动成功");
  return true;
}

// ---------------- WiFi 连接 ----------------
void connectWiFi() {
  Serial.print("连接 WiFi: ");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.setSleep(false);
  int t = 0;
  while (WiFi.status() != WL_CONNECTED && t < 40) {
    delay(500);
    Serial.print(".");
    t++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi 连接成功, IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\n❌ WiFi 连接失败");
  }
}

// ---------------- 拍照 + 调用多模态大模型 ----------------
void takePhotoAndAskDoubao() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("❌ 拍摄失败");
    return;
  }

  String base64Image = base64::encode(fb->buf, fb->len);

  // 资深导游话术提示词
  String prompt =
    "你现在化身从业多年的资深实地真人导游, 语气自然亲切, 像线下带着游客现场讲解一样。"
    "精准识别眼前这个景点, 详细完整讲述此地从古至今的完整发展历程、年代变迁、过往经历、"
    "流传已久的历史故事、发生在这里的经典事件、相关知名历史人物事迹、当地民间传说与风土人情, "
    "再介绍建筑由来、布局含义、背后渊源, 最后细致说明游玩看点、打卡位置和游玩注意事项, "
    "内容详实饱满、通俗易懂, 充满实景讲解氛围感, 不要简短概括, 尽量讲得细致全面有代入感。";

  if (!client.connect(DOUBAO_HOST, DOUBAO_PORT)) {
    Serial.println("❌ 连接服务器失败");
    esp_camera_fb_return(fb);
    return;
  }

  String postData = "{";
  postData += "\"model\":\"" + String(MODEL_NAME) + "\",";
  postData += "\"messages\":[{";
  postData += "\"role\":\"user\",";
  postData += "\"content\":[";
  postData += "{\"type\":\"text\",\"text\":\"" + prompt + "\"},";
  postData += "{\"type\":\"image_url\",\"image_url\":{\"url\":\"data:image/jpeg;base64," + base64Image + "\"}}";
  postData += "]}]}";

  client.println("POST /api/v3/chat/completions HTTP/1.1");
  client.println("Host: " + String(DOUBAO_HOST));
  client.println("Content-Type: application/json");
  client.println("Authorization: Bearer " + String(DOUBAO_API_KEY));
  client.println("Content-Length: " + String(postData.length()));
  client.println("Connection: close");
  client.println();
  client.println(postData);

  Serial.println("🤝 正在为您实景深度讲解, 请稍等...");
  String fullResponse = "";
  while (client.connected() || client.available()) {
    if (client.available()) {
      fullResponse += (char)client.read();
    }
  }

  int jsonStart = fullResponse.indexOf("{");
  int jsonEnd   = fullResponse.lastIndexOf("}");
  if (jsonStart != -1 && jsonEnd > jsonStart) {
    String jsonStr = fullResponse.substring(jsonStart, jsonEnd + 1);
    DynamicJsonDocument doc(16384);
    DeserializationError error = deserializeJson(doc, jsonStr);
    if (!error) {
      const char* res = doc["choices"][0]["message"]["content"].as<const char*>();
      Serial.println("\n==================== 实景导游全程讲解 ====================");
      Serial.println(res);
      Serial.println("==========================================================\n");
    } else {
      Serial.print("❌ JSON 解析失败: ");
      Serial.println(error.c_str());
    }
  } else {
    Serial.println("❌ 未在响应中找到 JSON 体");
  }

  esp_camera_fb_return(fb);
  client.stop();
}
