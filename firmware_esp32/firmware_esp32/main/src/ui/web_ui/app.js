const ws = new WebSocket("ws://esp32.local/ws");

ws.onmessage = (e) => {
  console.log("DATA:", e.data);

  if (e.data.includes("EMERGENCY")) {
    alert("⚠️ CẢNH BÁO KHẨN CẤP");
  }
};
