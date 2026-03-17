<?php
// Load cấu hình (BASE_URL, OLLAMA_URL, DEFAULT_ESP_IP, ...)
require_once '../app/config/config.php';

// Khởi tạo ứng dụng
require_once '../app/core/App.php';
require_once '../app/core/Controller.php';

$app = new App();
?>