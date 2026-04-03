import asyncio
from bleak import BleakClient, BleakScanner
from datetime import datetime

# ======================
# UUIDs of your sensor
# ======================
CHAR_UUIDS = {
    "temp":  "0000A001-0000-1000-8000-00805f9b34fb",
    "hum":   "0000A002-0000-1000-8000-00805f9b34fb",
    "press": "0000A003-0000-1000-8000-00805f9b34fb",
    "motion":"0000A004-0000-1000-8000-00805f9b34fb",
    "batt":  "00002A19-0000-1000-8000-00805f9b34fb"
}

sensor_device = None

# ======================
# Parse notifications
# ======================
def parse_sensor_data(uuid, data: bytearray):
    uuid = uuid.lower()
    if uuid == CHAR_UUIDS["temp"].lower():
        raw = int.from_bytes(data[0:2], "little", signed=True)
        return f"{raw / 100:.2f} °C"
    elif uuid == CHAR_UUIDS["hum"].lower():
        raw = int.from_bytes(data[0:2], "little")
        return f"{raw / 100:.2f} %"
    elif uuid == CHAR_UUIDS["press"].lower():
        raw = int.from_bytes(data[0:4], "little")
        return f"{raw} Pa"
    elif uuid == CHAR_UUIDS["motion"].lower():
        return "Motion detected\n"
    elif uuid == CHAR_UUIDS["batt"].lower():
        return f"{data[0]} %\n"
    else:
        return data

# ======================
# Notification handler
# ======================
def notification_handler(uuid, name):
    def handle(sender, data):
        val = parse_sensor_data(uuid, data)
        now = datetime.now().strftime("%H:%M:%S")
        print(f"[{now}] {name}: {val}")
    return handle

# ======================
# Connect and subscribe
# ======================
async def connect_and_subscribe(device):
    async with BleakClient(device) as client:
        print(f"\n✅ Connected to {device.name} [{device.address}]\n")
        for name, uuid in CHAR_UUIDS.items():
            await client.start_notify(uuid, notification_handler(uuid, name.capitalize()))
            print(f"Subscribed to {name.capitalize()} ({uuid})")
        print("\nListening for notifications...\n")
        while True:
            await asyncio.sleep(1)

# ======================
# Scan and find sensor
# ======================
async def find_sensor():
    global sensor_device
    while sensor_device is None:
        print("Scanning for BLE devices...")
        devices = await BleakScanner.discover(timeout=5.0)
        for d in devices:
            if d.name and "STM32WB" in d.name:
                sensor_device = d
                print(f"Sensor found at {d.address}, attempting connection...\n")
                return
        print("Sensor not found, retrying...\n")
        await asyncio.sleep(2)

# ======================
# Main BLE monitor
# ======================
async def run_ble_monitor():
    global sensor_device
    while True:
        await find_sensor()
        try:
            await connect_and_subscribe(sensor_device)
        except Exception as e:
            print(f"❌ Connection lost or error: {e}")
            sensor_device = None
            await asyncio.sleep(2)

# ======================
# Run
# ======================
if __name__ == "__main__":
    asyncio.run(run_ble_monitor())
