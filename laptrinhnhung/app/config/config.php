<?php
// Tự động nhận BASE_URL theo URL hiện tại (ổn định khi đổi tên thư mục hoặc hoa/thường)
if (!defined('BASE_URL')) {
	$isHttps = (!empty($_SERVER['HTTPS']) && $_SERVER['HTTPS'] !== 'off')
		|| (isset($_SERVER['SERVER_PORT']) && (int)$_SERVER['SERVER_PORT'] === 443);
	$scheme = $isHttps ? 'https' : 'http';
	$host = $_SERVER['HTTP_HOST'] ?? 'localhost';
	// index.php nằm trong /public
	$scriptDir = rtrim(str_replace('\\', '/', dirname($_SERVER['SCRIPT_NAME'] ?? '/public/index.php')), '/');
	define('BASE_URL', $scheme . '://' . $host . $scriptDir);
}

// Bật debug (chỉ dùng khi sửa lỗi, không bật khi deploy)
define('APP_DEBUG', false);

// Cấu hình AI
// Chọn nhà cung cấp AI: 'gemini' (Google AI Studio) hoặc 'ollama' (local)
define('AI_PROVIDER', 'gemini');

// --- Gemini (Google AI Studio / Gemini API) ---
// Cách an toàn nhất là set biến môi trường GEMINI_API_KEY trong Apache/PHP.
// Nếu cần test nhanh, bạn có thể điền thẳng key vào đây (không khuyến nghị khi public repo).
define('GEMINI_API_KEY', getenv('GEMINI_API_KEY') ?: '');
define('GEMINI_MODEL', 'gemini-2.5-flash');
define('GEMINI_API_BASE', 'https://generativelanguage.googleapis.com/v1beta');

// --- Google Cloud Text-to-Speech (server-side TTS) ---
// Dùng API key của Google Cloud project có bật "Cloud Text-to-Speech API"
define('GCP_TTS_API_KEY', getenv('GCP_TTS_API_KEY') ?: '');
define('GCP_TTS_VOICE_LANGUAGE', 'vi-VN');
// Vietnamese voice (Cloud TTS). Adjust if your project/region doesn't support Neural2.
define('GCP_TTS_VOICE_NAME', 'vi-VN-Standard-A');
define('GCP_TTS_AUDIO_ENCODING', 'MP3');

// --- Ollama (local) ---
define('OLLAMA_URL', 'http://localhost:11434/api/generate');
define('OLLAMA_MODEL', 'llama3'); // Hoặc 'gemma:2b', 'qwen:1.8b'

// --- ESP32 Gateway API ---
// Ưu tiên cấu hình bằng biến môi trường ESP_BASE_URL (vd: http://192.168.1.50)
// Fallback: địa chỉ ESP mặc định (có thể đổi trong Settings ở dashboard)
$espBaseUrl = getenv('ESP_BASE_URL') ?: (getenv('ESP32_BASE_URL') ?: 'http://192.168.215.110');
$espBaseUrl = rtrim($espBaseUrl, '/');
define('ESP_BASE_URL', $espBaseUrl);

// Backward-compat: code cũ dùng DEFAULT_ESP_IP
define('DEFAULT_ESP_IP', ESP_BASE_URL);
?>