#!/usr/bin/env python3
"""STM32 dual-servo control panel via pyserial (no browser needed)."""

from __future__ import annotations

import threading
import tkinter as tk
from tkinter import messagebox, ttk

import serial
from serial.tools import list_ports

DEFAULT_PORT = "COM7"
BAUD = 115200


class ServoPanel(tk.Tk):
    def __init__(self) -> None:
        super().__init__()
        self.title("舵机控制台 · STM32")
        self.geometry("520x560")
        self.minsize(460, 480)
        self.configure(bg="#17201c")

        self.ser: serial.Serial | None = None
        self._stop_reader = threading.Event()
        self._reader: threading.Thread | None = None

        style = ttk.Style(self)
        try:
            style.theme_use("clam")
        except tk.TclError:
            pass

        top = tk.Frame(self, bg="#17201c")
        top.pack(fill="x", padx=16, pady=14)

        tk.Label(
            top, text="舵机控制台", fg="#e8f0ea", bg="#17201c",
            font=("Microsoft YaHei UI", 18, "bold"),
        ).pack(anchor="w")
        tk.Label(
            top, text="直接连接 CH340 串口，不依赖浏览器",
            fg="#8ea396", bg="#17201c", font=("Microsoft YaHei UI", 10),
        ).pack(anchor="w", pady=(4, 0))

        row = tk.Frame(self, bg="#17201c")
        row.pack(fill="x", padx=16, pady=(4, 10))

        tk.Label(row, text="串口", fg="#e8f0ea", bg="#17201c").pack(side="left")
        self.port_var = tk.StringVar(value=DEFAULT_PORT)
        self.port_box = ttk.Combobox(row, textvariable=self.port_var, width=18, state="readonly")
        self.port_box.pack(side="left", padx=8)
        ttk.Button(row, text="刷新", command=self.refresh_ports).pack(side="left")

        self.conn_label = tk.Label(row, text="未连接", fg="#e06a6a", bg="#17201c")
        self.conn_label.pack(side="left", padx=12)

        self.btn_connect = ttk.Button(row, text="连接", command=self.toggle_connect)
        self.btn_connect.pack(side="right")

        grid = tk.Frame(self, bg="#17201c")
        grid.pack(fill="x", padx=16, pady=8)

        self._make_servo_block(grid, "舵机 1 · 左 (PA6)", 0, "S1")
        self._make_servo_block(grid, "舵机 2 · 右 (PA7)", 1, "S2")

        ttk.Button(self, text="全部回初始位 (HOME)", command=lambda: self.send("HOME")).pack(
            fill="x", padx=16, pady=(4, 10)
        )

        tk.Label(self, text="串口日志", fg="#8ea396", bg="#17201c", anchor="w").pack(
            fill="x", padx=16
        )
        self.log = tk.Text(
            self, height=14, bg="#0b100e", fg="#b7d0c0",
            insertbackground="#e8f0ea", relief="flat", font=("Consolas", 10),
        )
        self.log.pack(fill="both", expand=True, padx=16, pady=(4, 16))

        self.refresh_ports()
        self.protocol("WM_DELETE_WINDOW", self.on_close)
        self._append("就绪。选择 COM7 后点「连接」。")

    def _make_servo_block(self, parent: tk.Frame, title: str, col: int, prefix: str) -> None:
        box = tk.Frame(parent, bg="#203028", highlightbackground="#2a3a33", highlightthickness=1)
        box.grid(row=0, column=col, sticky="nsew", padx=(0 if col == 0 else 6, 0 if col == 1 else 6))
        parent.columnconfigure(col, weight=1)
        tk.Label(box, text=title, fg="#e8f0ea", bg="#203028", font=("Microsoft YaHei UI", 11, "bold")).pack(
            anchor="w", padx=12, pady=(10, 4)
        )
        btns = tk.Frame(box, bg="#203028")
        btns.pack(fill="x", padx=10, pady=(0, 12))
        for text, cmd in (("切换", prefix), ("初始位", f"{prefix}:0"), ("向内60°", f"{prefix}:1")):
            ttk.Button(btns, text=text, command=lambda c=cmd: self.send(c)).pack(
                side="left", expand=True, fill="x", padx=2
            )

    def refresh_ports(self) -> None:
        ports = [p.device for p in list_ports.comports()]
        if not ports:
            ports = [DEFAULT_PORT]
        self.port_box["values"] = ports
        if DEFAULT_PORT in ports:
            self.port_var.set(DEFAULT_PORT)
        elif ports:
            self.port_var.set(ports[0])
        self._append(f"可用串口: {', '.join(ports)}")

    def toggle_connect(self) -> None:
        if self.ser and self.ser.is_open:
            self.disconnect()
        else:
            self.connect()

    def connect(self) -> None:
        port = self.port_var.get().strip() or DEFAULT_PORT
        try:
            self.ser = serial.Serial(port, BAUD, timeout=0.1)
        except Exception as exc:  # noqa: BLE001
            messagebox.showerror("连接失败", str(exc))
            self._append(f"连接失败: {exc}")
            return

        self._stop_reader.clear()
        self._reader = threading.Thread(target=self._read_loop, daemon=True)
        self._reader.start()
        self.conn_label.config(text=f"已连接 {port}", fg="#3ecf8e")
        self.btn_connect.config(text="断开")
        self._append(f"已打开 {port} @ {BAUD}")

    def disconnect(self) -> None:
        self._stop_reader.set()
        if self.ser:
            try:
                self.ser.close()
            except Exception:  # noqa: BLE001
                pass
            self.ser = None
        self.conn_label.config(text="未连接", fg="#e06a6a")
        self.btn_connect.config(text="连接")
        self._append("已断开")

    def send(self, cmd: str) -> None:
        if not self.ser or not self.ser.is_open:
            messagebox.showwarning("未连接", "请先连接串口")
            return
        payload = (cmd + "\r\n").encode("ascii")
        try:
            self.ser.write(payload)
            self._append(f">> {cmd}")
        except Exception as exc:  # noqa: BLE001
            self._append(f"发送失败: {exc}")

    def _read_loop(self) -> None:
        buf = ""
        while not self._stop_reader.is_set() and self.ser and self.ser.is_open:
            try:
                data = self.ser.read(128)
            except Exception as exc:  # noqa: BLE001
                self.after(0, lambda: self._append(f"读取中断: {exc}"))
                break
            if not data:
                continue
            buf += data.decode("utf-8", errors="replace")
            while True:
                idx = buf.find("\n")
                if idx < 0:
                    break
                line = buf[:idx].rstrip("\r")
                buf = buf[idx + 1 :]
                if line:
                    self.after(0, lambda l=line: self._append(f"<< {l}"))

    def _append(self, msg: str) -> None:
        self.log.insert("end", msg + "\n")
        self.log.see("end")

    def on_close(self) -> None:
        self.disconnect()
        self.destroy()


if __name__ == "__main__":
    ServoPanel().mainloop()
