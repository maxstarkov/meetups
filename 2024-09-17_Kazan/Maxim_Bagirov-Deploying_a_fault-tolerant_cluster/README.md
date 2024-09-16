Добрый день, участники PG Bootcamp 2024 в Казани. В данном каталоге вы найдёте инструкцию по запуску решения, которое будет демонстрироваться в рамках мастер класса "Развертывание отказоустойчивого кластера с бекапированием и реплицированием в облачное хранилище".

# Требования к узлу, с которого будет разворачиваться решение
Минимально необходимыми требованиями являются:
* OS: Ubuntu 22.04
* RAM: 4096
* CPU: 4

Также необходима сетевая доступность между хостовой ОС и виртуальной машиной (в случае запуска в виртуальной машине).

# Демонстрация решения

<!-- Обратите внимание: в данном примере используется интерфейс enp0s8. В зависимости от имени интерфейса на Вашем устройстве его необходимо изменить (например на enp0s1) в скрипте node_prepare.sh -->

## Подготовка виртуальной машины к запуску решения
Для подготовки виртуальной машины, на которой будут подниматься docker контейнеры, выполните команды:
* Скачайте git репозиторий и перейдите в каталог, содержащий данные для МК:
```bash
su -
git clone https://github.com/TantorLabs/meetups.git
cd meetups/2024-09-17_Kazan/Maxim_Bagirov-Deploying_a_fault-tolerant_cluster
```
* Перейдите в привелегированный режим (если не сделали этого ранее), добавьте возможность исполнения для файла node_prepare.sh и запустите его
```bash
cd docker
chmod +x node_prepare.sh
./node_prepare.sh
```

## Запуск docker-compose файла ( выполняется из каталога docker )
```bash
docker compose up -d 
```

## Перезапуск службы sshd внутри контейнеров (выполняется из каталога docker)
После успешного запуска контейнеров перезагрузите sshd внутри каждого из них

```bash
./sshd_restart_in_docker.sh
```

## Подключение ssh-агента (выполняется из каталога docker)

```bash
ssh-agent bash
chmod 0600 ./id_rsa
ssh-add ./id_rsa
```

## Проверка доступность узлов для подключения по ssh (выполняется из каталога pg_cluster)
```bash
cd ../pg_cluster
ansible all -i inventory/my_inventory -m ping
```
## Заполнение переменных
Для запуска плейбука необходимо заполнить блок переменных, относящихся к S3 хранилищу. Заполните блок переменных, находящийся в файле ``pg_cluster/inventory/group_vars/patroni.yml``, данными актуальными для доступа к вашему S3: 
```bash
patroni_boostrap_method_walg_storage: "s3"
patroni_boostrap_method_walg_s3_username: ""
patroni_boostrap_method_walg_s3_password: ""
patroni_boostrap_method_walg_s3_bucket: ""
patroni_boostrap_method_walg_s3_region: ""
```

## Запуск ansible плейбука (выполняется из каталога pg_cluster)
Ниже приведены две команды для запуска разных версий СУБД внутри поднятых контейнеров и использование их в качестве основы для демонстрации решения:  
* для запуска решения, основанного на СУБД TantorDB, воспользуйтесь командой
```bash
ansible-playbook -i inventory/my_inventory -e "postgresql_vendor=tantordb edition=be major_version=15" pg-cluster.yaml -K -vvv
```
* для запуска решения, основанного на СУБД PostgreSQL (классической версии) 15, воспользуйтесь командой:
```bash
ansible-playbook -i inventory/my_inventory -e "postgresql_vendor=classic major_version=15" pg-cluster.yaml -K -vvv
```
<!-- Флаг -K нужен в случае, если у пользователя, от имени которого производится запуск, нет беспарольного доступа в root режим -->

# Полезные команды для работы с системой:
## Работа с ETCD

Просмотр состояния узла etcd (для примера выполняется на узле node1)
```bash
/opt/tantor/usr/bin/etcdctl --endpoints=https://10.10.44.101:2379 --cacert=/opt/tantor/etc/patroni/ca.pem --cert=/opt/tantor/etc/patroni/node1.pem --key=/opt/tantor/etc/patroni/node1-key.pem endpoint health
```

Просмотр узлов кластера (для примера выполняется на узле node1)
```bash
/opt/tantor/usr/bin/etcdctl --endpoints=https://10.10.44.101:2379 --cacert=/opt/tantor/etc/patroni/ca.pem --cert=/opt/tantor/etc/patroni/node1.pem --key=/opt/tantor/etc/patroni/node1-key.pem  member list
```

Удаление узла (для примера выполняется на узле node1)
```bash
/opt/tantor/usr/bin/etcdctl --endpoints=https://10.10.44.101:2379 --cacert=/opt/tantor/etc/patroni/ca.pem --cert=/opt/tantor/etc/patroni/node1.pem --key=/opt/tantor/etc/patroni/node1-key.pem  member remove <id>
```

Добавление нового узла в кластер etcd (для примера выполняется на узле node1)
```bash
/opt/tantor/usr/bin/etcdctl --endpoints=https://10.10.44.101:2379 --cacert=/opt/tantor/etc/patroni/ca.pem --cert=/opt/tantor/etc/patroni/node1.pem --key=/opt/tantor/etc/patroni/node1-key.pem  member add node3 --peer-urls=https://10.10.44.103:2380
```

## Работа с СУБД
Просмотр слотов репликации
```sql
SELECT * FROM pg_replication_slots;
```
Создание таблицы для теста
```sql
CREATE TABLE employees (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100),
    position VARCHAR(100),
    salary NUMERIC(10, 2)
);
```
Вставка данных в таблицу
```sql
INSERT INTO employees (name, position, salary) VALUES
('Alice', 'Developer', 60000),
('Bob', 'Manager', 80000),
('Charlie', 'Analyst', 55000);
```
Инициализация отправки wal-файлов в хранилище
```sql
SELECT pg_switch_wal();
```

## Работа с Patroni
Просмотр состояния кластера (для примера выполняется на узле node1)

```bash
/opt/tantor/usr/bin/patronictl -c /opt/tantor/etc/patroni/node1.yml list
```

Выполнение процедуры switchover
```bash
/opt/tantor/usr/bin/patronictl -c /opt/tantor/etc/patroni/node1.yml switchover
```