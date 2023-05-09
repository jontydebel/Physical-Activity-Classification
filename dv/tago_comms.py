import requests
import time
import threading
import data_training
import serial
import serial.tools.list_ports as st
from strip_ansi import strip_ansi
import json

max_pos = 4
min_pos = 0
ser = 0


def main():

    # ser = init_serial()
    model = data_training.load_and_train_regressor(50)

    while True:
        data_line = serial_readline()
        rssi_data = []
        try:
            for i in range(1, 9):
                key = f"node{i}"
                rssi_data.append(data_line[key])
        except Exception:
            print("passed")
            continue

        position = data_training.predict_pos(model, rssi_data)

        worker_thread = threading.Thread(target=send_to_dashboard, args=(data_line, position))
        worker_thread.start()
        time.sleep(9.2)
        print(threading.active_count())


def init_serial():
    # ======== CONNECT ==========
    try:
        ports = st.comports()
        name = ""

        for port in ports:
            print(f"device:{port.device} - name:{port.name} - description:{port.description} - hwid:{port.hwid}")
            hwid = port.hwid[12:16]
            if hwid == "C553":
                name = port.name
                print("selected " + name)
                break

        # name = "/dev/" + name
        ser = serial.Serial(name, 115200)
        return ser

    except Exception:
        time.sleep(1)
        init_serial()


def serial_readline():
    global ser
    try:
        ser.flushInput()
        dataline = ser.readline().decode().strip().replace(" ", "")
    except Exception:
        print("Reconnecting...")
        ser = init_serial()
    try:
        dataline = strip_ansi(dataline)
        dataline = dataline.replace("CSSE4011:~$", "")
        dataline = dataline[dataline.find("{"):]
        print(dataline)
        dataline_json = json.loads(dataline)
        return dataline_json
    except Exception:
        print("Failed to parse JSON")
        return


def send_to_dashboard(rssi_values: dict, position: tuple):
    token = "1265c444-e54c-4fd8-80b1-ea7423c26075"
    headers = {"Authorization": token}
    test_data = []
    for node, value in rssi_values.items():
        test_data.append({"variable": node, "value": value})

    x_pos, y_pos = position
    test_data.append({"variable": "x_pos", "value": round(x_pos, 2)})
    test_data.append({"variable": "y_pos", "value": round(y_pos, 2)})

    if x_pos < min_pos:
        x_pos = 0.1
    elif x_pos > max_pos:
        x_pos = 3.99

    if y_pos < min_pos:
        y_pos = 0.1
    elif y_pos > max_pos:
        y_pos = 3.99

    test_data.append({"variable": "position", "value": 1,
                      "metadata": {"x": x_pos / max_pos, "y": y_pos / max_pos}})

    response = requests.post("https://api.tago.io/data", headers=headers, json=test_data)

    print(response.text)


main()
