<?php
require_once '../app/config/config.php';

class AiController extends Controller {

    private function allowedCommandSet() {
        // Literal commands that the UI / gateway understands.
        $literals = [
            'LED1_ON','LED1_OFF',
            'LIGHT2_ON','LIGHT2_OFF',
            'LIGHT3_ON','LIGHT3_OFF',
            'LIGHTS_ALL_ON','LIGHTS_ALL_OFF',
            'LIGHTS_ALL_COLOR_YELLOW','LIGHTS_ALL_COLOR_RED','LIGHTS_ALL_COLOR_GREEN','LIGHTS_ALL_COLOR_BLUE','LIGHTS_ALL_COLOR_PURPLE','LIGHTS_ALL_COLOR_WHITE','LIGHTS_ALL_COLOR_RAINBOW',
            'FAN1_ON','FAN1_OFF','FAN2_ON','FAN2_OFF',
            'OFF',
            'CURTAIN_OPEN','CURTAIN_CLOSE','CURTAIN_STOP',
            'DOOR_OPEN','DOOR_CLOSE',
        ];

        $set = [];
        foreach ($literals as $c) {
            $set[$c] = true;
        }
        return $set;
    }

    private function sanitizeCommand($cmd) {
        if (!is_string($cmd)) return null;
        $c = trim($cmd);
        if ($c === '') return null;

        // Normalize some common variants that Gemini/speech may emit.
        $c = str_replace([' ', '-'], '_', $c);
        $c = strtoupper($c);
        $c = preg_replace('/_+/', '_', $c);

        // Accept parametrized commands for door PIN flows.
        if (preg_match('/^DOOR_PIN:(\d{1,10})$/', $c) === 1) return $c;
        if (preg_match('/^DOOR_PIN_SET:(\d{1,10})$/', $c) === 1) return $c;
        if (preg_match('/^DOOR_PIN_EMERGENCY_SET:(\d{1,10})$/', $c) === 1) return $c;

        // Backward-compat aliases
        if ($c === 'LIGHT1_ON') $c = 'LED1_ON';
        if ($c === 'LIGHT1_OFF') $c = 'LED1_OFF';

        $allowed = $this->allowedCommandSet();
        if (isset($allowed[$c])) return $c;

        return null;
    }

    private function sanitizeCommands($commands) {
        $raw = $this->extractCommandsFromMixed($commands);
        $out = [];
        foreach ($raw as $cmd) {
            $c = $this->sanitizeCommand($cmd);
            if (!$c) continue;
            // de-dup consecutive duplicates
            if (!empty($out) && end($out) === $c) continue;
            $out[] = $c;
        }
        return $out;
    }

    private function normText($s) {
        $t = trim((string)$s);
        if ($t === '') return '';
        if (function_exists('mb_strtolower')) {
            $t = mb_strtolower($t, 'UTF-8');
        } else {
            $t = strtolower($t);
        }
        // best-effort remove punctuation; keep vietnamese letters
        $t = preg_replace('/[^\p{L}\p{N}\s]/u', ' ', $t);
        $t = preg_replace('/\s+/u', ' ', $t);
        return trim($t);
    }

    private function normFold($s) {
        // Lowercase + remove diacritics (best-effort) + collapse whitespace.
        $t = trim((string)$s);
        if ($t === '') return '';
        if (function_exists('mb_strtolower')) {
            $t = mb_strtolower($t, 'UTF-8');
        } else {
            $t = strtolower($t);
        }

        // Prefer intl Normalizer if available.
        if (class_exists('Normalizer')) {
            try {
                $t = Normalizer::normalize($t, Normalizer::FORM_D);
                $t = preg_replace('/\p{Mn}+/u', '', $t);
            } catch (Exception $e) {
                // ignore
            }
        } else {
            // Fallback: iconv transliteration if available.
            if (function_exists('iconv')) {
                $tmp = @iconv('UTF-8', 'ASCII//TRANSLIT//IGNORE', $t);
                if (is_string($tmp) && $tmp !== '') {
                    $t = strtolower($tmp);
                }
            }
        }

        // Handle Vietnamese-specific letter
        $t = str_replace(['đ', 'Đ'], 'd', $t);

        $t = preg_replace('/[^a-z0-9\s]/i', ' ', $t);
        $t = preg_replace('/\s+/u', ' ', $t);
        return trim($t);
    }

    private function extractCommandsFromMixed($v) {
        // Accept: string (single or ';' separated) OR array of strings.
        if (is_array($v)) {
            $out = [];
            foreach ($v as $x) {
                if (!is_string($x)) continue;
                $c = trim($x);
                if ($c !== '') $out[] = $c;
            }
            return $out;
        }
        if (!is_string($v)) return [];
        $s = trim($v);
        if ($s === '') return [];
        // support separators: ; or newline
        $parts = preg_split('/\s*(?:;|\n|\r\n)\s*/u', $s);
        $out = [];
        foreach ($parts as $p) {
            $c = trim((string)$p);
            if ($c !== '') $out[] = $c;
        }
        return $out;
    }

    private function tryLocalIntent($userText) {
        // Fast, deterministic intents to avoid AI confusion.
        $raw = (string)$userText;
        $t = $this->normText($raw);
        $fold = $this->normFold($raw);
        if ($t === '' && $fold === '') return null;

        // --- Canned Q&A about Miki ---
        // Keep responses exactly as provided by the user.
        // Accept common variants from users/speech recognition.
        // Examples: "Miki là ai", "bạn là ai", "tên bạn là gì", "Miki là gì vậy".
        $askWho = (
            (strpos($fold, 'miki la ai') !== false)
            || (strpos($fold, 'ban la ai') !== false)
            || (strpos($fold, 'ban ten gi') !== false)
            || (strpos($fold, 'ten ban la gi') !== false)
            || ((strpos($fold, 'miki') !== false) && (strpos($fold, 'la gi') !== false))
            || ((strpos($fold, 'miki') !== false) && (strpos($fold, 'ten gi') !== false))
        );
        if ($askWho) {
            return [
                'reply' => 'MIKI Smarthome Assistant tôi là trợ lí nhà thông minh của bạn, không biết tôi có thể giúp gì cho bạn không.',
                'commands' => null,
                'device' => 'NONE',
                'action' => 'NONE'
            ];
        }

        $askMadeBy = (strpos($fold, 'ban do ai lam ra') !== false)
            || (strpos($fold, 'ban do ai tao ra') !== false)
            || (strpos($fold, 'ai lam ra ban') !== false)
            || (strpos($fold, 'ai tao ra ban') !== false);
        if ($askMadeBy) {
            return [
                'reply' => 'Tôi do nhóm sinh viên đến từ lớp 74DCTTT23 trường Đại học Công nghệ giao thông vận tải khoa Công nghệ thông tin thực hiện, phục vụ cho đồ án môn lập trình nhúng. thành viên nhóm bao gồm: Nguyễn Bá Duy (Nhóm trưởng), Nguyễn Hữu Giang, Lê Đức Minh, Trần Mỹ Anh, Nguyễn Thị Mỹ Phương. Tôi rất vui vì được hỗ trợ bạn!',
                'commands' => null,
                'device' => 'NONE',
                'action' => 'NONE'
            ];
        }

        $askHard = (strpos($fold, 'tao ra ban co kho khong') !== false)
            || (strpos($fold, 'de tao ra ban co kho khong') !== false)
            || (strpos($fold, 'tao ra ban kho khong') !== false);
        if ($askHard) {
            return [
                'reply' => 'Không khó, hiện nay với sự hỗ trợ cùng với AI và công nghệ, nhóm sinh viên đã làm ra tôi một cách nhanh chóng, nhưng vấn đề về ý tưởng mới là vấn đề quan trọng!',
                'commands' => null,
                'device' => 'NONE',
                'action' => 'NONE'
            ];
        }

        $isOnGlobal = (preg_match('/\b(bat|mo)\b/u', $fold) === 1);
        $isOffGlobal = (preg_match('/\b(tat)\b/u', $fold) === 1);

        // "tắt toàn bộ thiết bị" => Emergency Stop (OFF)
        $wantsAllDevicesOff = $isOffGlobal && (
            strpos($fold, 'tat toan bo thiet bi') !== false
            || strpos($fold, 'tat tat ca thiet bi') !== false
            || strpos($fold, 'tat het thiet bi') !== false
            || strpos($fold, 'tat toan bo cac thiet bi') !== false
            || strpos($fold, 'tat toan bo thiet bi dien') !== false
            || strpos($fold, 'tat toan bo cac thiet bi dien') !== false
            || strpos($fold, 'tat tat ca thiet bi dien') !== false
            || strpos($fold, 'tat het thiet bi dien') !== false
        );

        if ($wantsAllDevicesOff) {
            return [
                'reply' => 'Dạ, em đã tắt toàn bộ thiết bị rồi ạ.',
                'commands' => ['OFF'],
                'device' => 'ALL',
                'action' => 'OFF'
            ];
        }

        // --- Curtain intents (rèm/màn) ---
        $mentionsCurtain = (strpos($fold, 'rem') !== false) || (strpos($fold, 'man') !== false);
        if ($mentionsCurtain) {
            $wantsStop = (strpos($fold, 'dung') !== false) || (strpos($fold, 'tam dung') !== false) || (strpos($fold, 'ngung') !== false) || (strpos($fold, 'stop') !== false);
            $wantsOpen = (preg_match('/\bmo\b/u', $fold) === 1) || (strpos($fold, 'keo len') !== false) || (strpos($fold, 'keo ra') !== false);
            $wantsClose = (preg_match('/\bdong\b/u', $fold) === 1) || (strpos($fold, 'keo xuong') !== false) || (strpos($fold, 'keo vao') !== false);

            if ($wantsStop) {
                return ['reply' => 'Dạ, em đã gửi lệnh dừng rèm rồi ạ.', 'commands' => ['CURTAIN_STOP']];
            }
            if ($wantsOpen && !$wantsClose) {
                return ['reply' => 'Dạ, em đã gửi lệnh mở rèm rồi ạ.', 'commands' => ['CURTAIN_OPEN']];
            }
            if ($wantsClose && !$wantsOpen) {
                return ['reply' => 'Dạ, em đã gửi lệnh đóng rèm rồi ạ.', 'commands' => ['CURTAIN_CLOSE']];
            }
        }

        // --- Door intents (cửa/cổng) ---
        $mentionsDoor = (strpos($fold, 'cua') !== false) || (strpos($fold, 'cong') !== false);
        if ($mentionsDoor) {
            // If user says a PIN explicitly, prefer PIN command.
            if (preg_match('/\bmat\s*khau\s*(\d{1,10})\b/u', $fold, $m) === 1) {
                $pin = $m[1];
                return ['reply' => 'Dạ, em đã gửi lệnh mở cửa bằng mật khẩu rồi ạ.', 'commands' => ["DOOR_PIN:{$pin}"]];
            }
            $wantsOpenDoor = (preg_match('/\bmo\b/u', $fold) === 1);
            $wantsCloseDoor = (preg_match('/\bdong\b/u', $fold) === 1) || (strpos($fold, 'khoa') !== false);
            if ($wantsOpenDoor && !$wantsCloseDoor) {
                return ['reply' => 'Dạ, em đã gửi lệnh mở cửa rồi ạ.', 'commands' => ['DOOR_OPEN']];
            }
            if ($wantsCloseDoor && !$wantsOpenDoor) {
                return ['reply' => 'Dạ, em đã gửi lệnh đóng cửa rồi ạ.', 'commands' => ['DOOR_CLOSE']];
            }
        }

        // Helpful detectors that tolerate filler words from speech recognition
        $mentionsLight = (strpos($fold, 'den') !== false);
        $mentionsFan = (strpos($fold, 'quat') !== false);

        // Floors mentioned?
        $isThreeTiers = (preg_match('/\b(3\s*tang|ba\s*tang)\b/u', $fold) === 1);
        $isFloor3Only = (preg_match('/\b(tang\s*3|tang\s*ba)\b/u', $fold) === 1);
        $isFloor1Only = (preg_match('/\b(tang\s*1|tang\s*mot)\b/u', $fold) === 1);
        $isFloor2Only = (preg_match('/\b(tang\s*2|tang\s*hai)\b/u', $fold) === 1);

        // "bật đèn" / "bật toàn bộ đèn" / "bật đèn cả 3 tầng" (không chỉ tầng nào) => bật đèn 3 tầng
        $wantsLightsOnAll = $isOnGlobal && $mentionsLight && !$isFloor1Only && !$isFloor2Only && !$isFloor3Only && (
            // explicit all
            strpos($fold, 'bat tat ca den') !== false
            || strpos($fold, 'bat toan bo den') !== false
            || strpos($fold, 'bat den ca 3 tang') !== false
            || strpos($fold, 'bat den ca ba tang') !== false
            || strpos($fold, 'bat den 3 tang') !== false
            || strpos($fold, 'bat den ba tang') !== false
            // generic "bật ... đèn" with possible fillers between
            || (preg_match('/\bbat\b(?:\s+\w+){0,3}\s+\bden\b/u', $fold) === 1)
        );

        // "tắt đèn" / "tắt hết đèn" / "tắt đèn cả 3 tầng" (không chỉ tầng nào) => tắt toàn bộ đèn
        $wantsLightsOffAll = $isOffGlobal && $mentionsLight && !$isFloor1Only && !$isFloor2Only && !$isFloor3Only && (
            strpos($fold, 'tat het den') !== false
            || strpos($fold, 'tat den ca 3 tang') !== false
            || strpos($fold, 'tat den ca ba tang') !== false
            || strpos($fold, 'tat den 3 tang') !== false
            || strpos($fold, 'tat den ba tang') !== false
            || (preg_match('/\btat\b(?:\s+\w+){0,3}\s+\bden\b/u', $fold) === 1)
        );

        $wantsRainbow = (strpos($fold, 'nhieu mau') !== false)
            || (strpos($fold, 'rainbow') !== false)
            || (strpos($fold, 'ngu sac') !== false)
            || (strpos($fold, '7 mau') !== false);

        // IMPORTANT: disambiguation
        // - "đèn 3 tầng" / "3 tầng" => all 3 tiers (LIGHTS_ALL)
        // - "tầng 3" => only light3

        // Generic fan commands: "bật quạt" / "tắt quạt" => both fans
        if ($mentionsFan && ($isOnGlobal || $isOffGlobal)
            && (preg_match('/\bquat\b/u', $fold) === 1)
            && (preg_match('/\b(1|2|mot|hai|ba)\b/u', $fold) === 0) // no fan number
        ) {
            return [
                'reply' => $isOffGlobal ? 'Dạ, em đã tắt toàn bộ quạt rồi ạ.' : 'Dạ, em đã bật toàn bộ quạt rồi ạ.',
                'commands' => $isOffGlobal ? ['FAN1_OFF', 'FAN2_OFF'] : ['FAN1_ON', 'FAN2_ON'],
            ];
        }

        if ($wantsLightsOnAll && !$isFloor1Only && !$isFloor2Only && !$isFloor3Only) {
            return [
                'reply' => 'Dạ, em đã bật toàn bộ đèn (đèn 3 tầng) rồi ạ.',
                'commands' => ['LIGHTS_ALL_ON'],
                'device' => 'LIGHTS_ALL',
                'action' => 'ON'
            ];
        }

        if ($wantsLightsOffAll && !$isFloor1Only && !$isFloor2Only && !$isFloor3Only) {
            return [
                'reply' => 'Dạ, em đã tắt toàn bộ đèn rồi ạ.',
                'commands' => ['LIGHTS_ALL_OFF'],
                'device' => 'LIGHTS_ALL',
                'action' => 'OFF'
            ];
        }

        // Multi-command parsing for typical Vietnamese conjunctions.
        // Example: "Bật đèn tầng 1 và bật quạt 1" -> [LED1_ON, FAN1_ON]
        $clauses = preg_split('/\s*(?:\bva\b|\bvà\b|,|;|\.|\n|\r\n)\s*/u', $raw);
        if (is_array($clauses) && count($clauses) > 1) {
            $commands = [];
            foreach ($clauses as $cl) {
                $clRaw = trim((string)$cl);
                if ($clRaw === '') continue;
            $clNorm = $this->normText($clRaw);
            $clFold = $this->normFold($clRaw);

                $clOn = (preg_match('/\b(bat|mo)\b/u', $clFold) === 1);
                $clOff = (preg_match('/\b(tat)\b/u', $clFold) === 1);
                if (!$clOn && !$clOff) {
                    // If clause lacks explicit verb, fall back to global.
                    $clOn = $isOnGlobal;
                    $clOff = $isOffGlobal;
                }
                if (!$clOn && !$clOff) continue;

                $clWantsRainbow = (strpos($clFold, 'nhieu mau') !== false) || (strpos($clFold, 'rainbow') !== false);
                $clIsThreeTiers = (preg_match('/\b(3\s*tang|ba\s*tang)\b/u', $clFold) === 1);
                $clIsFloor1 = (preg_match('/\b(tang\s*1|tang\s*mot)\b/u', $clFold) === 1);
                $clIsFloor2 = (preg_match('/\b(tang\s*2|tang\s*hai)\b/u', $clFold) === 1);
                $clIsFloor3 = (preg_match('/\b(tang\s*3|tang\s*ba)\b/u', $clFold) === 1);

                if ($clWantsRainbow && ($clIsThreeTiers || strpos($clFold, 'den') !== false)) {
                    $commands[] = 'LIGHTS_ALL_ON';
                    $commands[] = 'LIGHTS_ALL_COLOR_RAINBOW';
                    continue;
                }

                // Fans
                // Support: "quạt 1" / "quạt số 1" / "quạt một"
                if (preg_match('/\bquat\b(?:\s+so)?\s*(?:1|mot)\b/u', $clFold) === 1) {
                    // Hardware mapping note: voice "quạt 1" controls FAN2
                    $commands[] = $clOff ? 'FAN2_OFF' : 'FAN2_ON';
                    continue;
                }
                // Support: "quạt 2" / "quạt số 2" / "quạt hai"
                if (preg_match('/\bquat\b(?:\s+so)?\s*(?:2|hai)\b/u', $clFold) === 1) {
                    // Hardware mapping note: voice "quạt 2" controls FAN1
                    $commands[] = $clOff ? 'FAN1_OFF' : 'FAN1_ON';
                    continue;
                }

                // Lights: 3-tier must win when explicitly mentioned
                if ($clIsThreeTiers) {
                    $commands[] = $clOff ? 'LIGHTS_ALL_OFF' : 'LIGHTS_ALL_ON';
                    continue;
                }
                if ($clIsFloor1) {
                    $commands[] = $clOff ? 'LED1_OFF' : 'LED1_ON';
                    continue;
                }
                if ($clIsFloor2) {
                    $commands[] = $clOff ? 'LIGHT2_OFF' : 'LIGHT2_ON';
                    continue;
                }
                if ($clIsFloor3) {
                    $commands[] = $clOff ? 'LIGHT3_OFF' : 'LIGHT3_ON';
                    continue;
                }
            }

            // de-dup consecutive duplicates
            $uniq = [];
            foreach ($commands as $c) {
                $c = trim((string)$c);
                if ($c === '') continue;
                if (!empty($uniq) && end($uniq) === $c) continue;
                $uniq[] = $c;
            }

            if (!empty($uniq)) {
                return [
                    'reply' => 'Dạ, em đã gửi lệnh theo yêu cầu rồi ạ.',
                    'commands' => $uniq,
                ];
            }
        }

        // Many-color for 3-tier lamp (single-clause)
        if ($wantsRainbow && ($isThreeTiers || strpos($t, 'den') !== false || strpos($raw, 'đèn') !== false)) {
            return [
                'reply' => 'Dạ, em đã bật đèn 3 tầng chế độ nhiều màu rồi ạ.',
                'commands' => ['LIGHTS_ALL_ON', 'LIGHTS_ALL_COLOR_RAINBOW'],
                'device' => 'LIGHTS_ALL',
                'action' => 'RAINBOW'
            ];
        }

        // On/off for 3-tier lamp
        if ($isThreeTiers && ($isOnGlobal || $isOffGlobal)) {
            $cmd = $isOffGlobal ? 'LIGHTS_ALL_OFF' : 'LIGHTS_ALL_ON';
            return [
                'reply' => $isOffGlobal ? 'Dạ, em đã tắt đèn 3 tầng rồi ạ.' : 'Dạ, em đã bật đèn 3 tầng rồi ạ.',
                'commands' => [$cmd],
                'device' => 'LIGHTS_ALL',
                'action' => $isOffGlobal ? 'OFF' : 'ON'
            ];
        }

        // On/off for specific floors (avoid confusing with 3-tier)
        if (!$isThreeTiers && ($isFloor1Only || $isFloor2Only || $isFloor3Only) && ($isOnGlobal || $isOffGlobal)) {
            $cmd = null;
            if ($isFloor1Only) $cmd = $isOffGlobal ? 'LED1_OFF' : 'LED1_ON';
            if ($isFloor2Only) $cmd = $isOffGlobal ? 'LIGHT2_OFF' : 'LIGHT2_ON';
            if ($isFloor3Only) $cmd = $isOffGlobal ? 'LIGHT3_OFF' : 'LIGHT3_ON';
            if ($cmd) {
                $label = $isFloor1Only ? 'tầng 1' : ($isFloor2Only ? 'tầng 2' : 'tầng 3');
                return [
                    'reply' => ($isOffGlobal ? 'Dạ, em đã tắt đèn ' : 'Dạ, em đã bật đèn ') . $label . ' rồi ạ.',
                    'commands' => [$cmd],
                ];
            }
        }

        return null;
    }

    private function ensureSession() {
        if (session_status() === PHP_SESSION_NONE) {
            session_start();
        }
    }
    
    public function index() {
        // Chặn truy cập trực tiếp
        header('HTTP/1.0 403 Forbidden');
    }

    public function process() {
        header('Content-Type: application/json');

        $this->ensureSession();

        // Hỗ trợ cả JSON body và form-data
        $rawInput = file_get_contents('php://input');
        $input = null;
        if (is_string($rawInput) && $rawInput !== '') {
            // Một số client (ví dụ PowerShell) có thể gửi body dạng UTF-16; chuyển về UTF-8 nếu phát hiện null byte
            if (strpos($rawInput, "\0") !== false && function_exists('mb_convert_encoding')) {
                $rawInputUtf8 = mb_convert_encoding($rawInput, 'UTF-8', 'UTF-16');
            } else {
                $rawInputUtf8 = $rawInput;
            }

            $input = json_decode($rawInputUtf8, true);
            // Nếu không phải JSON, thử parse dạng querystring
            if (!is_array($input)) {
                $tmp = [];
                parse_str($rawInputUtf8, $tmp);
                if (is_array($tmp) && !empty($tmp)) {
                    $input = $tmp;
                }
            }
        }

        $userText = (is_array($input) ? ($input['text'] ?? ($input['command'] ?? '')) : '')
            ?: ($_POST['text'] ?? ($_POST['command'] ?? ''));

        // Optional: allow client to send Gemini API key per request (more stable than relying on a prior sync + session).
        $clientGeminiKey = '';
        if (is_array($input) && isset($input['gemini_api_key'])) {
            $clientGeminiKey = trim((string)$input['gemini_api_key']);
        } elseif (isset($_POST['gemini_api_key'])) {
            $clientGeminiKey = trim((string)$_POST['gemini_api_key']);
        }
        if ($clientGeminiKey !== '') {
            $_SESSION['GEMINI_API_KEY'] = $clientGeminiKey;
        }

        $homeState = null;
        if (is_array($input) && isset($input['home_state']) && is_array($input['home_state'])) {
            $homeState = $input['home_state'];
        } elseif (isset($_POST['home_state']) && is_array($_POST['home_state'])) {
            $homeState = $_POST['home_state'];
        }

        if (empty($userText)) {
            $payload = [
                'status' => 'error',
                'message' => 'Thiếu nội dung (text).',
                // Minimal diagnostics (safe): helps debug body parsing / content-type issues
                'debug' => [
                    'method' => $_SERVER['REQUEST_METHOD'] ?? null,
                    'content_type' => $_SERVER['CONTENT_TYPE'] ?? ($_SERVER['HTTP_CONTENT_TYPE'] ?? null),
                    'raw_len' => is_string($rawInput) ? strlen($rawInput) : null,
                    'json_error' => function_exists('json_last_error_msg') ? json_last_error_msg() : null,
                ]
            ];

            echo json_encode($payload);
            return;
        }

        // 1) Ưu tiên parser cục bộ cho các câu lệnh điều khiển.
        //    Không bị throttle để bấm/nói liên tục vẫn nhận lệnh.
        $local = $this->tryLocalIntent($userText);
        if (is_array($local)) {
            $aiResult = json_encode($local, JSON_UNESCAPED_UNICODE);
        } else {
            // 2) Throttle chỉ áp dụng khi phải gọi AI (Gemini/Ollama) để tránh 429.
            $now = microtime(true);
            $last = $_SESSION['LAST_AI_CALL_AT'] ?? 0;
            if (is_numeric($last) && ($now - (float)$last) < 0.8) {
                echo json_encode([
                    'status' => 'success',
                    'response' => json_encode([
                        'reply' => 'Dạ anh/chị chờ em một chút rồi thử lại nhé.',
                        'command' => null,
                        'commands' => null
                    ], JSON_UNESCAPED_UNICODE),
                    'ai_action' => ['speak' => 'Dạ anh/chị chờ em một chút rồi thử lại nhé.', 'command' => null],
                    'reply' => 'Dạ anh/chị chờ em một chút rồi thử lại nhé.',
                    'command' => null,
                    'commands' => null
                ]);
                return;
            }
            $_SESSION['LAST_AI_CALL_AT'] = $now;

            // 3) Hỏi AI: chỉ sử dụng Gemini (theo yêu cầu)
            $aiResult = $this->askGemini($userText, $homeState);
        }

        // Chuẩn hoá output để tương thích UI hiện có (reply/command)
        // Đồng thời hỗ trợ schema mới (device/action/reply) giống smart_home_ai.py
        $aiDecoded = json_decode($aiResult, true) ?: [];
        $normalized = $this->normalizeAiOutput($aiDecoded);
        $reply = $normalized['reply'] ?? '';
        $command = $normalized['command'] ?? null;
        $commands = $normalized['commands'] ?? null;
        $device = $normalized['device'] ?? null;
        $action = $normalized['action'] ?? null;

        // Trả về string JSON "response" đã normalize để dashboard.php có thể JSON.parse() và dùng trực tiếp.
        $aiResult = json_encode([
            'reply' => $reply,
            'command' => $command,
            'commands' => $commands,
            'device' => $device,
            'action' => $action,
        ], JSON_UNESCAPED_UNICODE);

        // 2. Trả về kết quả cho Frontend xử lý (Nói và gọi ESP32)
        echo json_encode([
            'status' => 'success',
            // Phục vụ UI mới: chuỗi JSON gốc từ Ollama
            'response' => $aiResult,
            // Phục vụ UI cũ (public/js/app.js): trả thêm ai_action
            'ai_action' => [
                'speak' => $reply,
                'command' => $command
            ],
            // Thuận tiện cho frontend: trả thẳng reply/command đã parse
            'reply' => $reply,
            'command' => $command,
            'commands' => $commands,
            // Extra fields (không bắt buộc) cho schema device/action
            'device' => $device,
            'action' => $action
        ]);
    }

    public function saveApiKey() {
        header('Content-Type: application/json');
        $this->ensureSession();

        $raw = file_get_contents('php://input');
        $data = json_decode($raw, true);
        $apiKey = is_array($data) ? trim((string)($data['apiKey'] ?? '')) : '';

        if ($apiKey === '') {
            echo json_encode(['status' => 'error', 'message' => 'Missing apiKey']);
            return;
        }

        // Basic validation (Gemini keys usually start with AIza)
        if (strlen($apiKey) < 20) {
            echo json_encode(['status' => 'error', 'message' => 'API key có vẻ không hợp lệ (quá ngắn).']);
            return;
        }

        $_SESSION['GEMINI_API_KEY'] = $apiKey;
        echo json_encode(['status' => 'success']);
    }

    public function saveTtsKey() {
        header('Content-Type: application/json');
        $this->ensureSession();

        $raw = file_get_contents('php://input');
        $data = json_decode($raw, true);
        $apiKey = is_array($data) ? trim((string)($data['apiKey'] ?? '')) : '';

        if ($apiKey === '') {
            echo json_encode(['status' => 'error', 'message' => 'Missing apiKey']);
            return;
        }
        if (strlen($apiKey) < 20) {
            echo json_encode(['status' => 'error', 'message' => 'TTS API key có vẻ không hợp lệ (quá ngắn).']);
            return;
        }

        $_SESSION['GCP_TTS_API_KEY'] = $apiKey;
        echo json_encode(['status' => 'success']);
    }

    public function tts() {
        header('Content-Type: application/json');
        $this->ensureSession();

        $raw = file_get_contents('php://input');
        $data = json_decode($raw, true);
        $text = is_array($data) ? trim((string)($data['text'] ?? '')) : '';

        if ($text === '') {
            echo json_encode(['status' => 'error', 'message' => 'Missing text']);
            return;
        }

        $apiKey = $_SESSION['GCP_TTS_API_KEY'] ?? (defined('GCP_TTS_API_KEY') ? GCP_TTS_API_KEY : '');
        if (empty($apiKey)) {
            echo json_encode([
                'status' => 'error',
                'message' => 'Chưa cấu hình Google Cloud TTS API key.'
            ]);
            return;
        }

        $url = 'https://texttospeech.googleapis.com/v1/text:synthesize?key=' . urlencode($apiKey);
        $payload = [
            'input' => ['text' => $text],
            'voice' => [
                'languageCode' => defined('GCP_TTS_VOICE_LANGUAGE') ? GCP_TTS_VOICE_LANGUAGE : 'vi-VN',
                'name' => defined('GCP_TTS_VOICE_NAME') ? GCP_TTS_VOICE_NAME : 'vi-VN-Neural2-A'
            ],
            'audioConfig' => [
                'audioEncoding' => defined('GCP_TTS_AUDIO_ENCODING') ? GCP_TTS_AUDIO_ENCODING : 'MP3',
                // Keep it close to gTTS/Google Translate style.
                'speakingRate' => 1.0,
                'pitch' => 0.0,
                'volumeGainDb' => 0.0
            ]
        ];

        $ch = curl_init($url);
        curl_setopt($ch, CURLOPT_POST, 1);
        curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode($payload, JSON_UNESCAPED_UNICODE));
        curl_setopt($ch, CURLOPT_HTTPHEADER, ['Content-Type: application/json']);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($ch, CURLOPT_CONNECTTIMEOUT, 10);
        curl_setopt($ch, CURLOPT_TIMEOUT, 30);
        $response = curl_exec($ch);
        $httpCode = curl_getinfo($ch, CURLINFO_HTTP_CODE);
        $curlErr = curl_error($ch);
        curl_close($ch);

        if ($response === false || $httpCode >= 400) {
            $msg = $curlErr ?: ('HTTP ' . $httpCode . ' khi gọi Google Cloud TTS');
            $out = [
                'status' => 'error',
                'message' => 'Lỗi gọi Google Cloud TTS: ' . $msg
            ];
            if (defined('APP_DEBUG') && APP_DEBUG && is_string($response) && $response !== '') {
                $out['debug'] = ['response' => $response];
            }
            echo json_encode($out);
            return;
        }

        $json = json_decode($response, true);
        $audio = $json['audioContent'] ?? '';
        if (!$audio) {
            echo json_encode(['status' => 'error', 'message' => 'Google Cloud TTS không trả audioContent.']);
            return;
        }

        echo json_encode([
            'status' => 'success',
            'audioContent' => $audio,
            'audioEncoding' => defined('GCP_TTS_AUDIO_ENCODING') ? GCP_TTS_AUDIO_ENCODING : 'MP3'
        ]);
    }

    private function buildSystemPrompt($text, $homeState = null) {
        $homeStateJson = '';
        if (is_array($homeState) && !empty($homeState)) {
            $homeStateJson = json_encode($homeState, JSON_UNESCAPED_UNICODE);
        }

        $homeStateBlock = $homeStateJson
            ? ("HOME_STATE (latest sensor readings, may be null/unknown): " . $homeStateJson . "\n")
            : "HOME_STATE: not provided.\n";

        // Always use Vietnam timezone for time-aware responses
        $tz = 'Asia/Ho_Chi_Minh';
        try {
            $dt = new DateTimeImmutable('now', new DateTimeZone($tz));
            $nowIso = $dt->format(DateTimeInterface::ATOM);
        } catch (Exception $e) {
            // Fallback to server time if timezone is not available
            $nowIso = date('c');
            $tz = date_default_timezone_get();
        }
        $timeBlock = "CURRENT_TIME: {$nowIso} (timezone: {$tz})\n";

        return "Bạn là trợ lý ảo Smart Home tên Miki.\n"
            . "YÊU CẦU NGÔN NGỮ: Luôn trả lời 100% bằng Tiếng Việt, tự nhiên, lễ phép.\n"
            . "YÊU CẦU TRẢ LỜI: reply ngắn gọn (1 câu), thân thiện, phù hợp để đọc bằng giọng nói.\n"
            . "Bạn có thể trả lời câu hỏi thường ngày.\n"
            . $timeBlock
            . "Khi người dùng hỏi về cảm biến (nhiệt độ/độ ẩm/khí gas/cháy), hãy ưu tiên dùng HOME_STATE nếu có; nếu không có thì nói chưa có dữ liệu cảm biến thời gian thực.\n"
            . $homeStateBlock
            . "NHIỆM VỤ: Phân tích câu nói người dùng và xuất đúng 1 JSON object (không markdown, không thêm chữ).\n"
            . "JSON bắt buộc: {\"reply\":\"...\"} và có thể kèm \"command\" (1 lệnh) hoặc \"commands\" (nhiều lệnh).\n"
            . "QUY TẮC AN TOÀN: Nếu KHÔNG chắc chắn lệnh tương ứng, chỉ trả {\"reply\":\"...\"} và KHÔNG được bịa command/commands.\n"
            . "QUY TẮC AN TOÀN: Nếu có command/commands thì PHẢI đúng chính tả y hệt danh sách lệnh hợp lệ (in hoa + dấu gạch dưới).\n"
            . "- command: chuỗi lệnh duy nhất, ví dụ: \"LIGHT3_ON\"\n"
            . "- commands: mảng nhiều lệnh theo thứ tự, ví dụ: [\"LIGHTS_ALL_ON\",\"LIGHTS_ALL_COLOR_RAINBOW\"]\n"
            . "Ưu tiên dùng commands khi người dùng yêu cầu nhiều thao tác trong 1 câu.\n"
            . "Lệnh hợp lệ (command/commands): LED1_ON, LED1_OFF, LIGHT2_ON, LIGHT2_OFF, LIGHT3_ON, LIGHT3_OFF,\n"
            . "LIGHTS_ALL_ON, LIGHTS_ALL_OFF, LIGHTS_ALL_COLOR_YELLOW, LIGHTS_ALL_COLOR_RED, LIGHTS_ALL_COLOR_GREEN,\n"
            . "LIGHTS_ALL_COLOR_BLUE, LIGHTS_ALL_COLOR_PURPLE, LIGHTS_ALL_COLOR_WHITE, LIGHTS_ALL_COLOR_RAINBOW,\n"
            . "FAN1_ON, FAN1_OFF, FAN2_ON, FAN2_OFF, OFF, CURTAIN_OPEN, CURTAIN_CLOSE, CURTAIN_STOP, DOOR_OPEN, DOOR_CLOSE.\n"
            . "QUY TẮC PHÂN BIỆT: \"tầng 3\" là đèn tầng 3 (LIGHT3_*). \"đèn 3 tầng\" hoặc \"3 tầng\" là đèn 3 tầng (LIGHTS_ALL_*).\n"
            . "QUY TẮC ĐÈN: nếu người dùng nói \"tắt đèn\" (không nói tầng nào) thì coi là tắt toàn bộ đèn (LIGHTS_ALL_OFF).\n"
            . "QUY TẮC ĐÈN: nếu người dùng nói \"bật toàn bộ đèn\" hoặc \"bật tất cả đèn\" (không nói tầng nào) thì coi là bật đèn 3 tầng / toàn bộ đèn (LIGHTS_ALL_ON).\n"
            . "QUY TẮC THIẾT BỊ: nếu người dùng nói \"tắt toàn bộ thiết bị\" hoặc \"tắt tất cả thiết bị\" thì dùng Emergency Stop (OFF).\n"
            . "QUY TẮC QUẠT (mapping phần cứng): nếu người dùng nói \"quạt 1\" thì dùng lệnh FAN2_*; nếu nói \"quạt 2\" thì dùng lệnh FAN1_*.\n"
            . "Ví dụ:\n"
            . "- User: \"Bật đèn tầng 3\" -> {\"commands\":[\"LIGHT3_ON\"],\"reply\":\"Dạ, em đã bật đèn tầng 3 rồi ạ.\"}\n"
            . "- User: \"Bật đèn 3 tầng\" -> {\"commands\":[\"LIGHTS_ALL_ON\"],\"reply\":\"Dạ, em đã bật đèn 3 tầng rồi ạ.\"}\n"
            . "- User: \"Bật đèn 3 tầng nhiều màu\" -> {\"commands\":[\"LIGHTS_ALL_ON\",\"LIGHTS_ALL_COLOR_RAINBOW\"],\"reply\":\"Dạ, em đã bật đèn 3 tầng chế độ nhiều màu rồi ạ.\"}\n"
            . "- User: \"Tắt toàn bộ thiết bị\" -> {\"commands\":[\"OFF\"],\"reply\":\"Dạ, em đã tắt toàn bộ thiết bị rồi ạ.\"}\n"
            . "- User: \"Bật đèn tầng 1 và bật quạt 1\" -> {\"commands\":[\"LED1_ON\",\"FAN1_ON\"],\"reply\":\"Dạ, em đã bật đèn tầng 1 và quạt 1 rồi ạ.\"}\n"
                . "- User: \"Mở rèm\" -> {\"commands\":[\"CURTAIN_OPEN\"],\"reply\":\"Dạ, em đã gửi lệnh mở rèm rồi ạ.\"}\n"
                . "- User: \"Dừng rèm\" -> {\"commands\":[\"CURTAIN_STOP\"],\"reply\":\"Dạ, em đã gửi lệnh dừng rèm rồi ạ.\"}\n"
                . "- User: \"Đóng cửa\" -> {\"commands\":[\"DOOR_CLOSE\"],\"reply\":\"Dạ, em đã gửi lệnh đóng cửa rồi ạ.\"}\n"
            . "- User: \"Xin chào\" -> {\"reply\":\"Dạ em chào anh/chị, em có thể giúp gì ạ?\"}\n";
    }

    private function normalizeAiOutput($decoded) {
        if (!is_array($decoded)) {
            return ['reply' => '', 'command' => null, 'commands' => null, 'device' => null, 'action' => null];
        }

        $reply = trim((string)($decoded['reply'] ?? ''));

        // New schema: commands[]
        if (array_key_exists('commands', $decoded)) {
            $commands = $this->sanitizeCommands($decoded['commands']);
            $first = $commands[0] ?? null;
            return [
                'reply' => $reply,
                'command' => $first,
                'commands' => !empty($commands) ? $commands : null,
                'device' => $decoded['device'] ?? null,
                'action' => $decoded['action'] ?? null,
            ];
        }

        // Backward compatibility: nếu model trả thẳng command như phiên bản cũ
        if (array_key_exists('command', $decoded)) {
            $command = $decoded['command'];
            $command = is_string($command) ? trim($command) : $command;
            $commands = $this->sanitizeCommands($command);
            return [
                'reply' => $reply,
                'command' => $commands[0] ?? null,
                'commands' => !empty($commands) ? $commands : null,
                'device' => $decoded['device'] ?? null,
                'action' => $decoded['action'] ?? null,
            ];
        }

        $device = strtoupper(trim((string)($decoded['device'] ?? '')));
        $action = strtoupper(trim((string)($decoded['action'] ?? '')));

        if ($device === '' || $action === '') {
            return ['reply' => $reply, 'command' => null, 'commands' => null, 'device' => null, 'action' => null];
        }

        $command = null;
        $commands = [];
        switch ($device) {
            case 'LIGHT1':
                if ($action === 'ON') $command = 'LED1_ON';
                if ($action === 'OFF') $command = 'LED1_OFF';
                if ($action === 'TOGGLE') $command = 'LED1_ON';
                break;
            case 'LIGHT2':
                if ($action === 'ON') $command = 'LIGHT2_ON';
                if ($action === 'OFF') $command = 'LIGHT2_OFF';
                break;
            case 'LIGHT3':
                if ($action === 'ON') $command = 'LIGHT3_ON';
                if ($action === 'OFF') $command = 'LIGHT3_OFF';
                break;
            case 'LIGHTS_ALL':
            case 'LIGHTS3':
            case '3TANG':
                if ($action === 'ON') $command = 'LIGHTS_ALL_ON';
                if ($action === 'OFF') $command = 'LIGHTS_ALL_OFF';
                if ($action === 'RAINBOW') {
                    $commands = ['LIGHTS_ALL_ON', 'LIGHTS_ALL_COLOR_RAINBOW'];
                    $command = $commands[0];
                }
                break;
            case 'FAN1':
                // Hardware mapping note: FAN1 spoken/intended maps to FAN2 command.
                if ($action === 'TOGGLE') $command = 'FAN2_ON';
                if ($action === 'ON') $command = 'FAN2_ON';
                if ($action === 'OFF') $command = 'FAN2_OFF';
                break;
            case 'FAN2':
                // Hardware mapping note: FAN2 spoken/intended maps to FAN1 command.
                if ($action === 'TOGGLE') $command = 'FAN1_ON';
                if ($action === 'ON') $command = 'FAN1_ON';
                if ($action === 'OFF') $command = 'FAN1_OFF';
                break;
            case 'ALL':
                if ($action === 'OFF') $command = 'OFF';
                break;
            case 'MUSIC':
                if ($action === 'PLAY_1') $command = '1';
                if ($action === 'PLAY_2') $command = '2';
                if ($action === 'PLAY_3') $command = '3';
                if ($action === 'STOP') $command = '*';
                break;
            case 'VOLUME':
                if ($action === 'UP') $command = 'A';
                if ($action === 'DOWN') $command = 'B';
                break;
            case 'NONE':
                $command = null;
                break;
        }

        if ($command && empty($commands)) {
            $commands = [$command];
        }

        if (empty($commands)) {
            $commands = null;
        }

        return [
            'reply' => $reply,
            'command' => $command,
            'commands' => $commands,
            'device' => $device,
            'action' => $action,
        ];
    }

    private function extractFirstJsonObject($text) {
        // Gemini đôi khi trả về kèm text dư hoặc code-fence; mình tách JSON object đầu tiên.
        $start = strpos($text, '{');
        if ($start === false) return null;

        $depth = 0;
        $inString = false;
        $escape = false;
        $len = strlen($text);
        for ($i = $start; $i < $len; $i++) {
            $ch = $text[$i];
            if ($inString) {
                if ($escape) {
                    $escape = false;
                } elseif ($ch === '\\') {
                    $escape = true;
                } elseif ($ch === '"') {
                    $inString = false;
                }
                continue;
            }

            if ($ch === '"') {
                $inString = true;
                continue;
            }

            if ($ch === '{') $depth++;
            if ($ch === '}') {
                $depth--;
                if ($depth === 0) {
                    return substr($text, $start, $i - $start + 1);
                }
            }
        }
        return null;
    }

    private function askGemini($text, $homeState = null) {
        $this->ensureSession();
        $apiKey = $_SESSION['GEMINI_API_KEY'] ?? (defined('GEMINI_API_KEY') ? GEMINI_API_KEY : '');

        if (empty($apiKey)) {
            return json_encode([
                'reply' => 'Chưa cấu hình Gemini API Key. Vui lòng nhập trong mục Cài đặt (Gemini API Key) rồi thử lại.',
                'command' => null
            ], JSON_UNESCAPED_UNICODE);
        }

        // Tiny cache to reduce duplicate requests (helps avoid HTTP 429)
        $cacheKey = 'GEMINI_CACHE_' . md5($text . '|' . json_encode($homeState, JSON_UNESCAPED_UNICODE));
        $cached = $_SESSION[$cacheKey] ?? null;
        if (is_array($cached) && isset($cached['ts'], $cached['value']) && (time() - (int)$cached['ts']) < 30) {
            return (string)$cached['value'];
        }

        $systemPrompt = $this->buildSystemPrompt($text, $homeState);

        $basesToTry = [];
        $basesToTry[] = defined('GEMINI_API_BASE') ? rtrim(GEMINI_API_BASE, '/') : 'https://generativelanguage.googleapis.com/v1beta';
        // Fallback nếu v1beta bị 404 (một số môi trường đổi version)
        $basesToTry[] = 'https://generativelanguage.googleapis.com/v1';

        $basePayload = [
            'systemInstruction' => [
                'parts' => [['text' => $systemPrompt]]
            ],
            'contents' => [
                [
                    'role' => 'user',
                    'parts' => [['text' => $text]]
                ]
            ],
            'generationConfig' => [
                'temperature' => 0.2,
                'maxOutputTokens' => 256
            ]
        ];

        // Một số model/env không chấp nhận responseMimeType. Mình sẽ thử có trước, nếu lỗi thì tự fallback.
        $payloadWithJsonMime = $basePayload;
        $payloadWithJsonMime['generationConfig']['responseMimeType'] = 'application/json';

        $doRequest = function(string $url, array $payload) {
            $response = null;
            $httpCode = 0;
            $curlErr = '';

            // Retry/backoff on 429/503
            $attempts = 0;
            $maxAttempts = 3;
            while (true) {
                $attempts++;
                $ch = curl_init($url);
                curl_setopt($ch, CURLOPT_POST, 1);
                curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode($payload, JSON_UNESCAPED_UNICODE));
                curl_setopt($ch, CURLOPT_HTTPHEADER, ['Content-Type: application/json']);
                curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
                curl_setopt($ch, CURLOPT_CONNECTTIMEOUT, 10);
                curl_setopt($ch, CURLOPT_TIMEOUT, 30);
                $response = curl_exec($ch);
                $httpCode = curl_getinfo($ch, CURLINFO_HTTP_CODE);
                $curlErr = curl_error($ch);
                curl_close($ch);

                if ($response !== false && $httpCode < 400) {
                    return [$response, $httpCode, $curlErr];
                }
                if (($httpCode === 429 || $httpCode === 503) && $attempts < $maxAttempts) {
                    $sleepMs = $attempts === 1 ? 600 : 1500;
                    usleep($sleepMs * 1000);
                    continue;
                }
                return [$response, $httpCode, $curlErr];
            }
        };

        $response = null;
        $httpCode = 0;
        $curlErr = '';
        $usedBase = '';
        $usedPayloadHadMime = true;

        foreach ($basesToTry as $base) {
            $url = rtrim($base, '/') . '/models/' . GEMINI_MODEL . ':generateContent?key=' . urlencode($apiKey);
            $usedBase = $base;

            // 1) Try with responseMimeType first (better JSON discipline)
            [$response, $httpCode, $curlErr] = $doRequest($url, $payloadWithJsonMime);
            $usedPayloadHadMime = true;

            // If invalid argument / not supported, retry once without responseMimeType.
            if ($response !== false && $httpCode >= 400 && is_string($response) && $response !== '') {
                $errJson = json_decode($response, true);
                $detail = is_array($errJson) ? ($errJson['error']['message'] ?? ($errJson['message'] ?? '')) : '';
                $detailLower = strtolower((string)$detail);
                if ($httpCode === 400 && ($detailLower !== '') && (
                    strpos($detailLower, 'responsemimetype') !== false ||
                    strpos($detailLower, 'generationconfig') !== false ||
                    strpos($detailLower, 'invalid argument') !== false
                )) {
                    [$response, $httpCode, $curlErr] = $doRequest($url, $basePayload);
                    $usedPayloadHadMime = false;
                }
            }

            if ($response !== false && $httpCode < 400) {
                break;
            }

            // If 404, try next base (v1beta -> v1). Otherwise stop.
            if ($httpCode !== 404) {
                break;
            }
        }

        if ($response === false || $httpCode >= 400) {
            $detail = '';
            if (is_string($response) && $response !== '') {
                $errJson = json_decode($response, true);
                if (is_array($errJson)) {
                    $detail = $errJson['error']['message'] ?? ($errJson['message'] ?? '');
                }
            }

            if ($httpCode === 429) {
                $msg = $detail ?: 'Rate limit / quota exceeded.';
                $out = [
                    'reply' => 'Gemini đang giới hạn tần suất (HTTP 429). Vui lòng chờ 10–30 giây rồi thử lại. Nếu vẫn lặp lại, hãy kiểm tra quota/billing trên Google AI Studio hoặc giảm tần suất gửi lệnh. Chi tiết: ' . $msg,
                    'command' => null
                ];
            } else {
                $msg = $curlErr ?: ('HTTP ' . $httpCode . ' when calling Gemini');
                if ($detail) {
                    $msg .= ' - ' . $detail;
                }
                $out = [
                    'reply' => 'Gọi Gemini thất bại: ' . $msg,
                    'command' => null
                ];
            }

            if (defined('APP_DEBUG') && APP_DEBUG && is_string($response) && $response !== '') {
                $out['debug'] = ['response' => $response];
            }
            return json_encode($out, JSON_UNESCAPED_UNICODE);
        }

        $json = json_decode($response, true);
        $parts = $json['candidates'][0]['content']['parts'] ?? null;
        $textOut = '';
        if (is_array($parts)) {
            foreach ($parts as $p) {
                if (is_array($p) && isset($p['text']) && is_string($p['text'])) {
                    $textOut .= $p['text'];
                }
            }
        }
        $textOut = trim($textOut);
        if ($textOut === '') {
            $out = [
                'reply' => 'Gemini không trả về nội dung hợp lệ (không có text).',
                'command' => null
            ];
            if (defined('APP_DEBUG') && APP_DEBUG) {
                $out['debug'] = [
                    'used_base' => $usedBase,
                    'used_payload_responseMimeType' => $usedPayloadHadMime,
                    'raw' => $json
                ];
            }
            return json_encode($out, JSON_UNESCAPED_UNICODE);
        }

        $maybeJson = $this->extractFirstJsonObject($textOut) ?? $textOut;
        $decoded = json_decode($maybeJson, true);
        if (!is_array($decoded)) {
            // Fallback: coi toàn bộ là reply
            return json_encode([
                'reply' => trim($textOut),
                'command' => null
            ], JSON_UNESCAPED_UNICODE);
        }

        $reply = $decoded['reply'] ?? '';

        // Preserve multi-command outputs if the model returns them.
        $hasCommands = array_key_exists('commands', $decoded);
        $hasCommand = array_key_exists('command', $decoded);

        // Nếu model trả schema mới (device/action/reply) thì giữ nguyên để process() normalize.
        if (isset($decoded['device']) || isset($decoded['action'])) {
            $out = [
                'device' => $decoded['device'] ?? null,
                'action' => $decoded['action'] ?? null,
                'reply' => $reply,
            ];
            if ($hasCommands) $out['commands'] = $decoded['commands'];
            if ($hasCommand) $out['command'] = $decoded['command'];

            $final = json_encode($out, JSON_UNESCAPED_UNICODE);

            $_SESSION[$cacheKey] = ['ts' => time(), 'value' => $final];
            return $final;
        }

        $command = $decoded['command'] ?? null;
        $commands = $decoded['commands'] ?? null;

        // Đảm bảo reply luôn là tiếng Việt (phòng trường hợp model trả lẫn)
        // Nếu phát hiện nhiều ASCII letters, mình vẫn trả nhưng frontend sẽ đọc tiếng Việt; phần này chủ yếu dựa vào prompt.
        $final = json_encode([
            'reply' => $reply,
            'command' => $command,
            'commands' => $commands
        ], JSON_UNESCAPED_UNICODE);

        $_SESSION[$cacheKey] = ['ts' => time(), 'value' => $final];
        return $final;
    }

    private function askOllama($text, $homeState = null) {
        // Prompt được tinh chỉnh để khớp với các lệnh trong JS của bạn (LED1_ON, FAN, OFF...)
        $systemPrompt = $this->buildSystemPrompt($text, $homeState);

        // IMPORTANT: Ollama needs the actual user request in the prompt.
        // Previously we only sent instructions, which caused inconsistent outputs.
        $prompt = $systemPrompt
            . "\nUSER: \"" . str_replace('"', '\\"', (string)$text) . "\"\n"
            . "Hãy xuất JSON theo đúng schema ở trên.\n";

        $data = [
            "model" => OLLAMA_MODEL,
            "prompt" => $prompt,
            "stream" => false,
            "format" => "json"
        ];

        $ch = curl_init(OLLAMA_URL);
        curl_setopt($ch, CURLOPT_POST, 1);
        curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode($data));
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        $response = curl_exec($ch);
        curl_close($ch);

        $json = json_decode($response, true);
        return $json['response'] ?? '{}';
    }
}
?>