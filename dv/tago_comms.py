import requests
import time
import threading
import data_training
import serial
import serial.tools.list_ports as st
from strip_ansi import strip_ansi
import json
import pickle
import numpy as np

max_pos = 4
min_pos = 0
ser = 0
mapped_strings = {"sitting": ("Sitting", "brown", 0), "standing": ("Standing", "blue", 1),
                  "walking_slow": ("Walking (slow)", "green", 2), "walking_normal": ("Walking (normal)", "dark orange", 3),
                  "walking_fast": ("Walking (fast)", "purple", 4), "jogging": ("Jogging", "magenta", 5),
                  "running": ("Running", "red", 6)}


def main():
    # ser = init_serial()
    file = open("Model_KNN.obj", "rb")
    model = pickle.load(file)
    file.close()
    i = 0
    accel_gyro_data = []
    prev_result = ""
    while True:
        data_line = serial_readline()
        # print(data_line)
        if data_line:
            if ':' in data_line:
                data_line = data_line.split(":")
                data_line = [float(n) for n in data_line]
        # print(data_line[0])

        accel_gyro_row = []
        # headings = ["accel_x", "accel_y", "accel_z", "gyro_x", "gyro_y", "gyro_z"]
        try:
            for i in range(6):
                accel_gyro_row.append(data_line[i])
            accel_gyro_data.append(accel_gyro_row)
            if len(accel_gyro_data) > 60:
                accel_gyro_data = accel_gyro_data[-60:]
        except Exception:
            print("passed array build")
            continue

        transformed_data = transform_data(accel_gyro_data)

        result = data_training.predict_movement(model, transformed_data)
        print(f"MVMT_CLASS: {result}")
        if result != prev_result:
            worker_thread = threading.Thread(target=send_to_dashboard, args=(result[0],))
            worker_thread.start()
        prev_result = result
        # time.sleep(0.1)
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
        # ser = serial.Serial('/dev/tty.usbmodem142101', 115200)
        ser = serial.Serial(name, 115200)
        return ser

    except Exception:
        print("Failed serial")
        time.sleep(1)
        init_serial()


def transform_data(accel_gyro_raw):
    accel_gyro = np.array(accel_gyro_raw, dtype=float)

    # Vectorization
    accel_magnitude = np.sqrt(accel_gyro[:, 0] ** 2 + accel_gyro[:, 1] ** 2 + accel_gyro[:, 2] ** 2)
    gyro_magnitude = np.sqrt(accel_gyro[:, 3] ** 2 + accel_gyro[:, 4] ** 2 + accel_gyro[:, 5] ** 2)
    z_magnitude = accel_gyro[:, 2]

    # Aggregation
    accel_mean = accel_magnitude.mean()
    accel_stddev = accel_magnitude.std()
    gyro_mean = gyro_magnitude.mean()
    gyro_stddev = gyro_magnitude.std()
    z_mean = z_magnitude.mean()
    z_stddev = z_magnitude.std()
    # print(f"Z: {z_mean}")
    # print(f"ACCEL_MEAN: {accel_mean}")
    return [accel_mean, gyro_mean, accel_stddev, gyro_stddev, z_mean, z_stddev]


def serial_readline():
    global ser
    try:
        ser.flushInput()
        dataline = ser.readline().decode().strip().replace(" ", "")
    except Exception:
        print("Reconnecting...")
        ser = init_serial()
    try:
        # print(dataline)
        # dataline = strip_ansi(dataline)
        # dataline = dataline.replace("CSSE4011:~$", "")
        # print(dataline)
        # dataline = dataline[dataline.find("{"):]
        # print(dataline)
        # dataline_json = json.loads(dataline)

        dataline = strip_ansi(dataline)
        dataline = dataline.replace("CSSE4011:~$", "")
        dataline = dataline[dataline.find("Datais"):]
        dataline = dataline.replace("Datais", "")
        return dataline
    except Exception as e:
        print(e)
        print("Failed to parse JSON")
        return


def send_to_dashboard(state):
    token = "1265c444-e54c-4fd8-80b1-ea7423c26075"
    headers = {"Authorization": token}
    text, colour, speed = mapped_strings[state]
    test_data = [{"variable": "state", "value": text,
                  "metadata": {
                      "color": colour,
                  }
                  },
                 {"variable": "speed", "value": speed,
                  "metadata": {
                      "color": colour
                  }
                  },
                 {"variable": "remainder_speed", "value": 6-speed,
                  "metadata": {
                      "color": "grey"
                  }}]
    response = requests.post("https://api.tago.io/data", headers=headers, json=test_data)
    print(response.text)


main()
