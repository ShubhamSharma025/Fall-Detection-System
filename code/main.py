import websocket
import json
from datetime import datetime
import pandas as pd
from twilio.rest import Client


account_sid = 'YOUR_SID'
auth_token = 'YOUR_AUTH_TOKEN'
twilio_from = '+1234567890' 
twilio_to = '+919876543210'  
twilio_client = Client(account_sid, auth_token)


log = []

def send_alert(message):
    twilio_client.messages.create(
        body=message,
        from_=twilio_from,
        to=twilio_to
    )

def on_message(ws, message):
    try:
        data = json.loads(message)
        acc = data.get("acc", 0)
        angle = data.get("angle", 0)
        status = data.get("status", "Normal")
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

       
        print(f"{timestamp} | Accel: {acc:.2f} G | Angle: {angle:.1f}° | Status: {status}")

        
        log.append({
            "Timestamp": timestamp,
            "AccelMagnitude": acc,
            "Angle": angle,
            "Status": status
        })

        if status.lower() == "fall":
            send_alert(f"⚠️ Fall detected at {timestamp} | Accel: {acc:.2f} | Angle: {angle:.1f}°")

    except Exception as e:
        print("❌ Error:", e)

def on_open(ws):
    print("✅ Connected to ESP32 WebSocket")

def on_close(ws, code, msg):
    print("🔌 Connection closed")

def on_error(ws, error):
    print("❗ WebSocket Error:", error)

def start_ws():
    ws = websocket.WebSocketApp("ws://192.168.4.1/ws",
                                on_open=on_open,
                                on_message=on_message,
                                on_close=on_close,
                                on_error=on_error)
    ws.run_forever()

try:
    start_ws()
except KeyboardInterrupt:
    print("📁 Saving dataset...")
    df = pd.DataFrame(log)
    df.to_csv("fall_detection_log.csv", index=False)
    print("✅ Data saved to fall_detection_log.csv")
