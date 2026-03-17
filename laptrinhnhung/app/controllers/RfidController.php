<?php

class RfidController extends Controller {
    public function proxy() {
        header('Content-Type: application/json; charset=utf-8');

        if (($_SERVER['REQUEST_METHOD'] ?? '') !== 'POST') {
            http_response_code(405);
            echo json_encode(['ok' => false, 'message' => 'Method not allowed']);
            return;
        }

        $raw = file_get_contents('php://input');
        $body = json_decode($raw, true);
        if (!is_array($body)) {
            http_response_code(400);
            echo json_encode(['ok' => false, 'message' => 'Invalid JSON body']);
            return;
        }

        $gscriptUrl = trim((string)($body['gscript_url'] ?? ''));
        $payload = $body['payload'] ?? new stdClass();

        if ($gscriptUrl === '') {
            http_response_code(400);
            echo json_encode(['ok' => false, 'message' => 'Chưa cấu hình Google Apps Script URL']);
            return;
        }

        // Basic allowlist to avoid proxying arbitrary URLs.
        $u = parse_url($gscriptUrl);
        $host = isset($u['host']) ? strtolower($u['host']) : '';
        $path = isset($u['path']) ? (string)$u['path'] : '';

        $allowedHost = ($host === 'script.google.com' || $host === 'script.googleusercontent.com');
        $looksLikeWebApp = (strpos($path, '/macros/s/') !== false) && (substr($path, -5) === '/exec');

        if (!$allowedHost || !$looksLikeWebApp) {
            http_response_code(400);
            echo json_encode(['ok' => false, 'message' => 'Apps Script URL không hợp lệ (cần link WebApp /exec)']);
            return;
        }

        $json = json_encode($payload);
        if ($json === false) {
            http_response_code(400);
            echo json_encode(['ok' => false, 'message' => 'Payload không hợp lệ']);
            return;
        }

        if (!function_exists('curl_init')) {
            http_response_code(500);
            echo json_encode(['ok' => false, 'message' => 'PHP cURL chưa được bật (xem php.ini)']);
            return;
        }

        $call = function(string $method, string $url, ?string $body = null, array $headers = []) {
            $ch = curl_init($url);
            curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
            curl_setopt($ch, CURLOPT_FOLLOWLOCATION, true);
            curl_setopt($ch, CURLOPT_CONNECTTIMEOUT, 5);
            curl_setopt($ch, CURLOPT_TIMEOUT, 12);

            if (strtoupper($method) === 'POST') {
                curl_setopt($ch, CURLOPT_POST, true);
                $hs = $headers ?: ['Content-Type: application/json'];
                curl_setopt($ch, CURLOPT_HTTPHEADER, $hs);
                curl_setopt($ch, CURLOPT_POSTFIELDS, (string)$body);
            } else {
                curl_setopt($ch, CURLOPT_HTTPGET, true);
            }

            $resp = curl_exec($ch);
            $errNo = curl_errno($ch);
            $errMsg = curl_error($ch);
            $http = (int)curl_getinfo($ch, CURLINFO_HTTP_CODE);
            curl_close($ch);
            return [$resp, $http, $errNo, $errMsg];
        };

        // Flatten scalar params once for form/GET fallbacks.
        $arr = [];
        if (is_array($payload)) {
            $arr = $payload;
        } elseif (is_object($payload)) {
            $arr = get_object_vars($payload);
        }

        $flat = [];
        foreach ($arr as $k => $v) {
            if (is_scalar($v) || $v === null) {
                $flat[(string)$k] = $v === null ? '' : (string)$v;
            }
        }

        // Try POST(JSON) first.
        [$resp, $http, $errNo, $errMsg] = $call('POST', $gscriptUrl, $json, ['Content-Type: application/json', 'Cache-Control: no-cache']);

        if ($resp === false) {
            // If POST failed at transport level, we still try GET fallback below.
            $resp = '';
        }

        $trim = trim((string)$resp);
        $data = null;
        if ($trim !== '' && $trim !== 'null') {
            $data = json_decode($resp, true);
        }

        $action = isset($flat['action']) ? strtolower(trim((string)$flat['action'])) : '';
        $forceJson = isset($flat['_force_json']) && ((string)$flat['_force_json'] === '1' || strtolower((string)$flat['_force_json']) === 'true');
        $isWrite = preg_match('/^rfid_(add|create|update|edit|del|delete|remove)$/i', $action) === 1;

        // IMPORTANT:
        // Many Apps Script implementations do JSON.parse(e.postData.contents) for doPost.
        // Sending form bodies like "uid=..." will throw "Unexpected token 'u'".
        // So we only attempt form/GET fallbacks for non-write actions (list/history/last), unless explicitly forced.
        $allowFallback = (!$forceJson) && (!$isWrite);

        $needsGetFallback = false;
        if ($allowFallback) {
            if ($resp === false || $trim === '' || $trim === 'null') {
                $needsGetFallback = true;
            } else if (!is_array($data) && !is_object($data)) {
                $needsGetFallback = true;
            } else {
                // Some deployments reply 200 with {ok:false,message:"Unknown action"} when method/params mismatch.
                $msg = '';
                if (is_array($data) && isset($data['message'])) $msg = (string)$data['message'];
                if (is_object($data) && isset($data->message)) $msg = (string)$data->message;
                if (preg_match('/unknown\s+action/i', $msg) || preg_match('/method\s+not\s+allowed/i', $msg)) {
                    $needsGetFallback = true;
                }
            }
        }

        if ($needsGetFallback) {
            // Many GAS doPost(e) scripts rely on e.parameter (form-encoded), not JSON.
            $qs = http_build_query($flat);

            [$respF, $httpF, $errNoF, $errMsgF] = $call(
                'POST',
                $gscriptUrl,
                $qs,
                ['Content-Type: application/x-www-form-urlencoded; charset=utf-8', 'Cache-Control: no-cache']
            );

            if ($respF !== false) {
                $resp = $respF;
                $http = $httpF;
                $errNo = $errNoF;
                $errMsg = $errMsgF;
                $trim = trim((string)$resp);
                $data = null;
                if ($trim !== '' && $trim !== 'null') {
                    $data = json_decode($resp, true);
                }
            }

            // If still not usable, fall back to doGet-style query string.
            $stillBad = false;
            if ($resp === false || $trim === '' || $trim === 'null') {
                $stillBad = true;
            } else if (!is_array($data) && !is_object($data)) {
                $stillBad = true;
            } else {
                $msg2 = '';
                if (is_array($data) && isset($data['message'])) $msg2 = (string)$data['message'];
                if (is_object($data) && isset($data->message)) $msg2 = (string)$data->message;
                if (preg_match('/unknown\s+action/i', $msg2) || preg_match('/method\s+not\s+allowed/i', $msg2)) {
                    $stillBad = true;
                }
            }

            if ($stillBad) {
                $url = $gscriptUrl . (strpos($gscriptUrl, '?') === false ? '?' : '&') . $qs;
                [$resp2, $http2, $errNo2, $errMsg2] = $call('GET', $url, null, ['Cache-Control: no-cache']);
                if ($resp2 !== false) {
                    $resp = $resp2;
                    $http = $http2;
                    $errNo = $errNo2;
                    $errMsg = $errMsg2;
                    $trim = trim((string)$resp);
                    $data = null;
                    if ($trim !== '' && $trim !== 'null') {
                        $data = json_decode($resp, true);
                    }
                }
            }
        }

        if ($resp === false) {
            http_response_code(502);
            echo json_encode(['ok' => false, 'message' => 'Không gọi được Apps Script: ' . ($errMsg ?: ('cURL error ' . $errNo))]);
            return;
        }

        // If still not JSON, wrap it so the frontend can show a useful error.
        if ($trim !== '' && $trim !== 'null' && !is_array($data) && !is_object($data)) {
            http_response_code(502);
            echo json_encode([
                'ok' => false,
                'message' => 'Apps Script không trả JSON hợp lệ',
                'http' => $http,
                'raw' => mb_substr($trim, 0, 400)
            ]);
            return;
        }

        if ($http >= 400) {
            http_response_code($http);
        } else {
            http_response_code(200);
        }

        // Pass-through JSON as-is if possible.
        if ($trim === '' || $trim === 'null') {
            echo json_encode(['ok' => false, 'message' => 'Apps Script trả rỗng']);
            return;
        }
        echo $resp;
    }
}
