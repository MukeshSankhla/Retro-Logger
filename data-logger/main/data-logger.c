#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_tls.h"
#include "cJSON.h"
#include "esp_crt_bundle.h"

// UART Configuration
#define UART_PORT_NUM      UART_NUM_2
#define UART_BAUD_RATE     115200
#define UART_RX_PIN        16
#define UART_TX_PIN        17
#define UART_BUF_SIZE      (1024 * 6)

// LED Pin Definitions
#define POWER_LED_PIN         13  // Power Indicator
#define WIFI_LED_PIN          14  // WiFi Indicator
#define UART_DATA_LED_PIN     25  // UART Data Indicator
#define FIREBASE_LED_PIN      26  // Firebase Success Indicator

// LED Blink Configurations
#define LED_BLINK_DURATION_MS 200   // Duration for LED to stay on during blink
#define WIFI_BLINK_INTERVAL   500   // WiFi connecting blink interval

// WiFi credentials
#define WIFI_SSID "Your_SSID"
#define WIFI_PASS "Your_PASSWORD"

// Firebase configuration
#define FIREBASE_HOST "https://your-project.firebaseio.com/"
#define FIREBASE_SECRET "your_firebase_secret"

static const char *TAG = "FIREBASE_ESP32";

// FreeRTOS event group and queue
static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;
QueueHandle_t uart_queue;

// LED status task handle
TaskHandle_t wifi_led_task_handle = NULL;

// Root CA certificate
const char *google_root_ca = 
"-----BEGIN CERTIFICATE-----\n"
"MIIFVzCCAz+gAwIBAgINAgPlk28xsBNJiGuiFzANBgkqhkiG9w0BAQwFADBHMQsw\n"
"CQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEU\n"
"MBIGA1UEAxMLR1RTIFJvb3QgUjEwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAw\n"
"MDAwWjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZp\n"
"Y2VzIExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjEwggIiMA0GCSqGSIb3DQEBAQUA\n"
"A4ICDwAwggIKAoICAQC2EQKLHuOhd5s73L+UPreVp0A8of2C+X0yBoJx9vaMf/vo\n"
"27xqLpeXo4xL+Sv2sfnOhB2x+cWX3u+58qPpvBKJXqeqUqv4IyfLpLGcY9vXmX7w\n"
"Cl7raKb0xlpHDU0QM+NOsROjyBhsS+z8CZDfnWQpJSMHobTSPS5g4M/SCYe7zUjw\n"
"TcLCeoiKu7rPWRnWr4+wB7CeMfGCwcDfLqZtbBkOtdh+JhpFAz2weaSUKK0Pfybl\n"
"qAj+lug8aJRT7oM6iCsVlgmy4HqMLnXWnOunVmSPlk9orj2XwoSPwLxAwAtcvfaH\n"
"szVsrBhQf4TgTM2S0yDpM7xSma8ytSmzJSq0SPly4cpk9+aCEI3oncKKiPo4Zor8\n"
"Y/kB+Xj9e1x3+naH+uzfsQ55lVe0vSbv1gHR6xYKu44LtcXFilWr06zqkUspzBmk\n"
"MiVOKvFlRNACzqrOSbTqn3yDsEB750Orp2yjj32JgfpMpf/VjsPOS+C12LOORc92\n"
"wO1AK/1TD7Cn1TsNsYqiA94xrcx36m97PtbfkSIS5r762DL8EGMUUXLeXdYWk70p\n"
"aDPvOmbsB4om3xPXV2V4J95eSRQAogB/mqghtqmxlbCluQ0WEdrHbEg8QOB+DVrN\n"
"VjzRlwW5y0vtOUucxD/SVRNuJLDWcfr0wbrM7Rv1/oFB2ACYPTrIrnqYNxgFlQID\n"
"AQABo0IwQDAOBgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4E\n"
"FgQU5K8rJnEaK0gnhS9SZizv8IkTcT4wDQYJKoZIhvcNAQEMBQADggIBAJ+qQibb\n"
"C5u+/x6Wki4+omVKapi6Ist9wTrYggoGxval3sBOh2Z5ofmmWJyq+bXmYOfg6LEe\n"
"QkEzCzc9zolwFcq1JKjPa7XSQCGYzyI0zzvFIoTgxQ6KfF2I5DUkzps+GlQebtuy\n"
"h6f88/qBVRRiClmpIgUxPoLW7ttXNLwzldMXG+gnoot7TiYaelpkttGsN/H9oPM4\n"
"7HLwEXWdyzRSjeZ2axfG34arJ45JK3VmgRAhpuo+9K4l/3wV3s6MJT/KYnAK9y8J\n"
"ZgfIPxz88NtFMN9iiMG1D53Dn0reWVlHxYciNuaCp+0KueIHoI17eko8cdLiA6Ef\n"
"MgfdG+RCzgwARWGAtQsgWSl4vflVy2PFPEz0tv/bal8xa5meLMFrUKTX5hgUvYU/\n"
"Z6tGn6D/Qqc6f1zLXbBwHSs09dR2CQzreExZBfMzQsNhFRAbd03OIozUhfJFfbdT\n"
"6u9AWpQKXCBfTkBdYiJ23//OYb2MI3jSNwLgjt7RETeJ9r/tSQdirpLsQBqvFAnZ\n"
"0E6yove+7u7Y/9waLd64NnHi/Hm3lCXRSHNboTXns5lndcEZOitHTtNCjv0xyBZm\n"
"2tIMPNuzjsmhDYAPexZ3FL//2wmUspO8IFgV6dtxQ/PeEMMA3KgqlbbC1j+Qa3bb\n"
"bP6MvPJwNQzcmRk13NfIRmPVNnGuV/u3gm3c\n"
"-----END CERTIFICATE-----\n";

// LED initialization function
void init_leds(void)
{
    // Configure LED GPIOs as outputs
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << POWER_LED_PIN) | (1ULL << WIFI_LED_PIN) | 
                         (1ULL << UART_DATA_LED_PIN) | (1ULL << FIREBASE_LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // Turn on Power LED
    gpio_set_level(POWER_LED_PIN, 1);
    
    // All other LEDs start OFF
    gpio_set_level(WIFI_LED_PIN, 0);
    gpio_set_level(UART_DATA_LED_PIN, 0);  
    gpio_set_level(FIREBASE_LED_PIN, 0);
    
    ESP_LOGI(TAG, "LEDs initialized");
}

// Function to blink LED temporarily
void blink_led(int pin) {
    gpio_set_level(pin, 1);
    vTaskDelay(LED_BLINK_DURATION_MS / portTICK_PERIOD_MS);
    gpio_set_level(pin, 0);
}

// Task to handle WiFi LED status
void wifi_led_task(void *pvParameters) {
    bool connected = false;
    
    while(1) {
        EventBits_t bits = xEventGroupGetBits(wifi_event_group);
        connected = (bits & WIFI_CONNECTED_BIT) != 0;
        
        if (connected) {
            // WiFi connected - solid light
            gpio_set_level(WIFI_LED_PIN, 1);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        } else {
            // WiFi disconnected or connecting - blinking
            gpio_set_level(WIFI_LED_PIN, 1);
            vTaskDelay(WIFI_BLINK_INTERVAL / portTICK_PERIOD_MS);
            gpio_set_level(WIFI_LED_PIN, 0);
            vTaskDelay(WIFI_BLINK_INTERVAL / portTICK_PERIOD_MS);
        }
    }
}

// WiFi event handler
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Connection failed! Retrying...");
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        // WiFi LED will be handled by wifi_led_task
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        // WiFi LED will be handled by wifi_led_task
    }
}

// Initialize WiFi as station
void wifi_init_sta(void)
{
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "Connecting to %s...", WIFI_SSID);
    
    // Start WiFi LED task
    xTaskCreate(wifi_led_task, "wifi_led_task", 2048, NULL, 5, &wifi_led_task_handle);
    
    // Wait for WiFi connection
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                           WIFI_CONNECTED_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to AP SSID: %s", WIFI_SSID);
    } else {
        ESP_LOGE(TAG, "Failed to connect to AP SSID: %s", WIFI_SSID);
    }
}

// HTTP event handler
esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (evt->data_len) {
                printf("%.*s", evt->data_len, (char*)evt->data);
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT");
            break;
        default:
            ESP_LOGI(TAG, "Unhandled HTTP event: %d", evt->event_id);
            break;
    }
    return ESP_OK;
}

void send_to_firebase(const char *json_data) {
    // Parse the JSON to get the key
    cJSON *root = cJSON_Parse(json_data);
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return;
    }
    
    // Get the first object (should be "winding", "assembling", "testing", or "machine_error")
    cJSON *first_obj = NULL;
    cJSON *child = root->child;
    if (child) {
        first_obj = child;
    }
    
    if (first_obj == NULL) {
        ESP_LOGE(TAG, "Invalid JSON structure");
        cJSON_Delete(root);
        return;
    }
    
    // Extract the path to use
    const char *category = first_obj->string;
    
    // Construct Firebase URL with the category path
    char url[256];
    snprintf(url, sizeof(url), "https://%s/%s.json?auth=%s", 
             FIREBASE_HOST, category, FIREBASE_AUTH);
    
    ESP_LOGI(TAG, "Sending data to Firebase: %s", json_data);
    ESP_LOGI(TAG, "URL: %s", url);
    
    // Configure HTTP client
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_PATCH,  // PATCH method to update without replacing
        .event_handler = http_event_handler,
        .timeout_ms = 10000,
        .cert_pem = google_root_ca,
        .skip_cert_common_name_check = false
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    
    // Get the data portion (without the category wrapper)
    char *data_to_send = cJSON_PrintUnformatted(first_obj);
    
    esp_http_client_set_post_field(client, data_to_send, strlen(data_to_send));
    
    esp_err_t err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP Status = %d", esp_http_client_get_status_code(client));
        // Blink Firebase success LED
        blink_led(FIREBASE_LED_PIN);
    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
    }
    
    free(data_to_send);
    esp_http_client_cleanup(client);
    cJSON_Delete(root);
}

void uart_init() {
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE, UART_BUF_SIZE, 10, &uart_queue, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

void uart_event_task(void *pvParameters) {
    uart_event_t event;
    static char buffer[UART_BUF_SIZE];  // Static buffer to accumulate data
    static int buffer_pos = 0;          // Position in the buffer
    uint8_t *temp_buf = (uint8_t *) malloc(UART_BUF_SIZE);
    
    while (1) {
        if (xQueueReceive(uart_queue, (void *)&event, portMAX_DELAY)) {
            switch (event.type) {
                case UART_DATA:
                    // Read the bytes from UART
                    int len = uart_read_bytes(UART_PORT_NUM, temp_buf, event.size, portMAX_DELAY);
                    temp_buf[len] = '\0';  // Ensure null termination
                    ESP_LOGI(TAG, "Received data chunk: %s", temp_buf);
                    
                    // Blink UART data LED
                    blink_led(UART_DATA_LED_PIN);
                    
                    // Append data to the buffer
                    if (buffer_pos + len < UART_BUF_SIZE - 1) {
                        memcpy(buffer + buffer_pos, temp_buf, len);
                        buffer_pos += len;
                        buffer[buffer_pos] = '\0';  // Null-terminate
                    } else {
                        // Buffer overflow, reset
                        ESP_LOGE(TAG, "Buffer overflow, resetting");
                        buffer_pos = 0;
                        continue;
                    }
                    
                    // Check if we have a complete JSON object
                    // A simple check is to look for matching braces
                    int braces = 0;
                    bool in_string = false;
                    
                    for (int i = 0; i < buffer_pos; i++) {
                        if (buffer[i] == '"' && (i == 0 || buffer[i-1] != '\\')) {
                            in_string = !in_string;
                        }
                        if (!in_string) {
                            if (buffer[i] == '{') braces++;
                            if (buffer[i] == '}') braces--;
                        }
                    }
                    
                    // If we have a complete JSON object
                    if (braces == 0 && buffer_pos > 0) {
                        ESP_LOGI(TAG, "Received complete JSON: %s", buffer);
                        
                        // Validate JSON before sending
                        cJSON *json = cJSON_Parse(buffer);
                        if (json != NULL) {
                            send_to_firebase(buffer);
                            cJSON_Delete(json);
                        } else {
                            ESP_LOGE(TAG, "Invalid JSON received: %s", buffer);
                        }
                        
                        // Reset the buffer
                        buffer_pos = 0;
                    }
                    break;
                    
                case UART_FIFO_OVF:
                    ESP_LOGI(TAG, "UART FIFO overflow");
                    uart_flush_input(UART_PORT_NUM);
                    xQueueReset(uart_queue);
                    break;
                    
                case UART_BUFFER_FULL:
                    ESP_LOGI(TAG, "UART buffer full");
                    uart_flush_input(UART_PORT_NUM);
                    xQueueReset(uart_queue);
                    break;
                    
                default:
                    ESP_LOGI(TAG, "UART event type: %d", event.type);
                    break;
            }
        }
    }
    
    free(temp_buf);
    vTaskDelete(NULL);
}

void app_main(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize LEDs
    init_leds();
    
    // Initialize WiFi
    wifi_init_sta();
    
    // Initialize UART
    uart_init();
    
    // Create task to handle UART events
    xTaskCreate(uart_event_task, "uart_event_task", 4096, NULL, 12, NULL);
    
    ESP_LOGI(TAG, "System initialized. Waiting for UART data...");
}
