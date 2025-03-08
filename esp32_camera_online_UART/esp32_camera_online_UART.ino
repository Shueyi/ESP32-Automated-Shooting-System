#include <monkeyrepellent_inferencing.h>
#include "edge-impulse-sdk/dsp/image/image.hpp"
#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>

// WiFi Credentials
const char* WIFI_SSID = "Key in Your WIFI Username";
const char* WIFI_PASS = "Key in Your WIFI Password";


// Camera model configuration
#define CAMERA_MODEL_AI_THINKER

// Camera pin definitions
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
#endif

// Constants for camera and image settings
#define EI_CAMERA_RAW_FRAME_BUFFER_COLS           320
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS           240
#define EI_CAMERA_FRAME_BYTE_SIZE                 3

// Web Server configuration
WebServer server(80);

// Global variables
static bool debug_nn = false;
static bool is_initialised = false;
uint8_t *snapshot_buf;

// Camera configuration
static camera_config_t camera_config = {
    .pin_pwdn = PWDN_GPIO_NUM,
    .pin_reset = RESET_GPIO_NUM,
    .pin_xclk = XCLK_GPIO_NUM,
    .pin_sscb_sda = SIOD_GPIO_NUM,
    .pin_sscb_scl = SIOC_GPIO_NUM,
    .pin_d7 = Y9_GPIO_NUM,
    .pin_d6 = Y8_GPIO_NUM,
    .pin_d5 = Y7_GPIO_NUM,
    .pin_d4 = Y6_GPIO_NUM,
    .pin_d3 = Y5_GPIO_NUM,
    .pin_d2 = Y4_GPIO_NUM,
    .pin_d1 = Y3_GPIO_NUM,
    .pin_d0 = Y2_GPIO_NUM,
    .pin_vsync = VSYNC_GPIO_NUM,
    .pin_href = HREF_GPIO_NUM,
    .pin_pclk = PCLK_GPIO_NUM,
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_JPEG,
    .frame_size = FRAMESIZE_QVGA,
    .jpeg_quality = 12,
    .fb_count = 1,
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

// Global variable to store latest predictions
String latestPredictions = "No predictions yet";

// HTML for predictions page
const char* PREDICTIONS_HTML = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Object Detection</title>
    <style>
        body { font-family: Arial, sans-serif; text-align: center; }
        #predictions { 
            background-color: #f0f0f0; 
            margin: 10px auto; 
            padding: 10px; 
            max-width: 600px; 
            white-space: pre-wrap;
        }
    </style>
</head>
<body>
    <h1>ESP32 Object Detection Predictions</h1>
    <div id="predictions"></div>

    <script>
        function updatePredictions() {
            fetch('/predictions')
                .then(response => response.text())
                .then(data => {
                    document.getElementById('predictions').innerText = data;
                });
        }
        setInterval(updatePredictions, 1000);
        updatePredictions();
    </script>
</body>
</html>
)rawliteral";

// Function prototypes
bool ei_camera_init(void);
bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf);
static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr);

// Server handlers
void handleRoot() {
    server.send(200, "text/html", PREDICTIONS_HTML);
}

void handlePredictions() {
    server.send(200, "text/plain", latestPredictions);
}

void setup() {
    Serial.begin(115200);
    Serial1.begin(9600, SERIAL_8N1, 3, 1);

    if (!ei_camera_init()) {
        Serial.println("Failed to initialize Camera!");
        return;
    }

    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");
    Serial.println("ESP32 Object Detection Server at: " + WiFi.localIP().toString());

    server.on("/", handleRoot);
    server.on("/predictions", handlePredictions);
    server.begin();
}

void loop() {
    server.handleClient();

    if (ei_sleep(5) != EI_IMPULSE_OK) return;

    snapshot_buf = (uint8_t*)malloc(EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE);
    if (!snapshot_buf) {
        Serial.println("ERR: Failed to allocate snapshot buffer!");
        return;
    }

    ei::signal_t signal;
    signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    signal.get_data = &ei_camera_get_data;

    if (!ei_camera_capture(EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT, snapshot_buf)) {
        Serial.println("Failed to capture image");
        free(snapshot_buf);
        return;
    }

    ei_impulse_result_t result = { 0 };
    EI_IMPULSE_ERROR err = run_classifier(&signal, &result, debug_nn);
    if (err != EI_IMPULSE_OK) {
        Serial.printf("Classifier error: %d\n", err);
        free(snapshot_buf);
        return;
    }

    String predictionStr = "Predictions:\n";
#if EI_CLASSIFIER_OBJECT_DETECTION == 1
    if (result.bounding_boxes_count > 0) {
        predictionStr += "Bounding Boxes:\n";
        for (uint32_t i = 0; i < result.bounding_boxes_count; i++) {
            ei_impulse_result_bounding_box_t bb = result.bounding_boxes[i];
            if (bb.value > 0) {
                // Add bounding box details to the predictions string
                predictionStr += String(bb.label) + " (" + String(bb.value, 2) + 
                                 "): x:" + String(bb.x) + 
                                 " y:" + String(bb.y) + 
                                 " w:" + String(bb.width) + 
                                 " h:" + String(bb.height) + "\n";
                
                // Send coordinates via UART
                String message = String(bb.x) + "," + String(bb.y) + "\n";
                Serial1.print(message);

                // Debug output
                Serial.print("Sent via UART: ");
                Serial.println(message);
            }
        }
    } else {
        predictionStr += "No bounding boxes detected.\n";
    }
#else
    for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        predictionStr += String(ei_classifier_inferencing_categories[i]) + ": " + 
                         String(result.classification[i].value, 4) + "\n";
    }
#endif
#if EI_CLASSIFIER_HAS_ANOMALY
    predictionStr += "Anomaly: " + String(result.anomaly, 3) + "\n";
#endif

    latestPredictions = predictionStr;
    Serial.println(latestPredictions);

    free(snapshot_buf);
}


bool ei_camera_init(void) {
    if (is_initialised) return true;

    esp_err_t err = esp_camera_init(&camera_config); 
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        return false;
    }

    sensor_t * s = esp_camera_sensor_get();
    if (s->id.PID == OV3660_PID) {
        s->set_vflip(s, 1);
        s->set_brightness(s, 1);
        s->set_saturation(s, 0);
    }
    is_initialised = true;
    return true;
}

bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) return false;

    bool converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, snapshot_buf);
    esp_camera_fb_return(fb);

    if (!converted) return false;

    if ((img_width != EI_CAMERA_RAW_FRAME_BUFFER_COLS) || (img_height != EI_CAMERA_RAW_FRAME_BUFFER_ROWS)) {
        ei::image::processing::crop_and_interpolate_rgb888(
            out_buf, EI_CAMERA_RAW_FRAME_BUFFER_COLS, EI_CAMERA_RAW_FRAME_BUFFER_ROWS,
            out_buf, img_width, img_height);
    }
    return true;
}

static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr) {
    size_t pixel_ix = offset * 3;
    size_t pixels_left = length;
    size_t out_ptr_ix = 0;

    while (pixels_left != 0) {
        out_ptr[out_ptr_ix] = (snapshot_buf[pixel_ix + 2] << 16) + (snapshot_buf[pixel_ix + 1] << 8) + snapshot_buf[pixel_ix];
        out_ptr_ix++;
        pixel_ix += 3;
        pixels_left--;
    }
    return 0;
}
