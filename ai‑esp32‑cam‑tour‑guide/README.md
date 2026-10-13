# AI-ESP32-CAM 实景人文智能导游

> 基于 ESP32-CAM + 火山方舟多模态大模型实现的低成本实景 AI 导游：拍照识别景点，自动生成真人风格人文讲解。

![platform](https://img.shields.io/badge/platform-ESP32-blue)
![arduino](https://img.shields.io/badge/Arduino-ESP32-green)
![license](https://img.shields.io/badge/license-MIT-lightgrey)

## 📖 项目简介

针对传统人工导游成本高、讲解参差不齐，以及商用智能导览设备价格昂贵、内容固化的行业痛点，本项目以百元级 ESP32-CAM 为硬件核心，采用端云协同架构：设备拍摄实景照片 → 编码上传至云端多模态大模型 → 实时输出包含历史沿革、人物典故、民间传说、游玩建议的沉浸式导游文本。

适合景区、博物馆、乡村文旅场景，以及高校嵌入式 / 人工智能方向的创新创业实践。

### ✨ 核心亮点

1. **低成本嵌入式多模态全链路**：整机硬件百元以内，完成「图像采集 → Base64 编码 → 云端多模态识别 → 人文内容生成」整套流程。
2. **沉浸式人文叙事**：非预录语音，大模型实时生成口语化、故事化讲解，高度还原真人导游现场感。
3. **工程优化**：针对 ESP32 内存受限做图像压缩、超时控制，规避内存溢出与请求超时问题。

## 🧰 硬件清单

- ESP32-CAM 开发板（OV2640 摄像头模块）
- FTDI-232 下载器（烧录用）
- 5V / 2A 供电电源或 USB 供电

## ⚙️ 环境依赖

Arduino IDE 中需要安装：

- ESP32 开发板支持包（`espressif/arduino-esp32`）
- `ArduinoJson` 库
- `base64` 库（ESP32 core 已内置，一般无需单独安装）

## 🚀 快速开始

1. 打开 `src/ai_tour_guide.ino`，修改文件顶部「用户配置区」：
   - `WIFI_SSID` / `WIFI_PASSWORD`：你的 WiFi 名称与密码
   - `DOUBAO_API_KEY`：火山方舟 API Key
   - `MODEL_NAME`：你的多模态模型 Endpoint ID（形如 `ep-xxxxxxxx`）
2. Arduino IDE 选择开发板 **AI Thinker ESP32-CAM**，上传固件。
3. 打开串口监视器，波特率 `115200`。
4. 在串口输入框输入 `p` 并回车，触发拍照，等待 AI 返回讲解文本。

## 📁 仓库结构

```
ai-esp32-cam-tour-guide/
├── src/                  # Arduino 主程序
│   └── ai_tour_guide.ino
├── docs/                 # 项目研究报告（docx + markdown 版）
├── assets/               # 接线图、实物照片（自行补充）
├── .gitignore
└── README.md
```

## 📄 文档

完整项目研究报告见：

- `docs/基于ESP32与多模态大模型的AI智能导游项目研究报告.docx`
- `docs/report-readme.md`（GitHub 网页直接可读的 Markdown 版）

## 🔮 后续迭代方向

- 外接 DFPlayer / MAX98357 音频模块，文本转语音播放
- 增加麦克风，实现语音指令触发拍照
- 离线缓存历史讲解文本
- 多语言讲解、后台管理 Web 页面

## 📝 License

MIT
