#!/bin/bash

# Создаём директорию для установки Python
sudo mkdir -p /opt/tantor/python

# Обновляем список пакетов и устанавливаем зависимости для сборки Python
sudo apt-get update
sudo apt-get install -y build-essential libssl-dev zlib1g-dev libbz2-dev \
    libreadline-dev libsqlite3-dev wget curl llvm libncurses5-dev libncursesw5-dev \
    xz-utils tk-dev libffi-dev liblzma-dev

# Переходим в директорию установки
cd /opt/tantor/python

# Скачиваем и распаковываем исходники Python
sudo wget https://www.python.org/ftp/python/3.12.4/Python-3.12.4.tgz
sudo tar -xf Python-3.12.4.tgz

# Переходим в директорию исходников Python
cd Python-3.12.4

# Конфигурируем сборку с ensurepip
sudo ./configure --enable-optimizations --with-ensurepip=install

# Собираем и устанавливаем Python
sudo make -j$(nproc)
sudo make altinstall

# Устанавливаем Ansible с помощью pip
sudo python3.12 -m pip install ansible

# Устанавливаем Docker
sudo apt-get update
sudo apt-get install -y ca-certificates curl gnupg

# Настраиваем репозиторий Docker
sudo install -m 0755 -d /etc/apt/keyrings
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo gpg --dearmor -o /etc/apt/keyrings/docker.gpg
sudo chmod a+r /etc/apt/keyrings/docker.gpg

echo \
  "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/ubuntu \
  $(. /etc/os-release && echo "$VERSION_CODENAME") stable" | \
  sudo tee /etc/apt/sources.list.d/docker.list > /dev/null

sudo apt-get update
sudo apt-get install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin

# Добавляем текущего пользователя в группу docker
sudo usermod -aG docker $USER

# Настраиваем IPTables
sudo iptables -t nat -A POSTROUTING -s 10.10.44.0/24 ! -d 10.10.44.0/24 -o enp0s8 -j MASQUERADE
sudo iptables -I FORWARD -s 10.10.44.0/24 -j ACCEPT
sudo iptables -I FORWARD -d 10.10.44.0/24 -j ACCEPT

# Создаём сеть macvlan
sudo ip link add macvlan0 link enp0s8 type macvlan mode bridge
sudo ip addr add 10.10.44.10/32 dev macvlan0
sudo ip link set macvlan0 up
sudo ip route add 10.10.44.0/24 dev macvlan0

# Добавляем nameserver 8.8.8.8 в /etc/resolv.conf
echo "nameserver 8.8.8.8" | sudo tee /etc/resolv.conf > /dev/null

# Включаем форвардинг IP
sudo sed -i '/^#*net.ipv4.ip_forward/s/^#*//; s/=.*/=1/' /etc/sysctl.conf
if ! grep -q '^net.ipv4.ip_forward' /etc/sysctl.conf; then
    echo 'net.ipv4.ip_forward = 1' | sudo tee -a /etc/sysctl.conf > /dev/null
fi

# Применяем изменения
sudo sysctl -p
