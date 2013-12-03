#!/bin/sh
### expected directory structure
DIR_BASE=`pwd $`
FILE_HDA=${DIR_BASE}/hda
FILE_CONF=${DIR_BASE}/conf
FILE_OVERRIDE=${DIR_BASE}/override
### run directory will be auto-created with following files
DIR_RUN=${DIR_BASE}/run
FILE_MONITOR=${DIR_RUN}/monitor
FILE_PID=${DIR_RUN}/pid
FILE_OUT=${DIR_RUN}/out
### user-defined variables in ./kvm.conf
#GUEST_ID       [required]      generate the mac and tap name
#GUEST_MEMORY   [optional]      in mega-bytes, default to 1024
GUEST_MEMORY=1024
if [ -f ${FILE_CONF} ]
then
        . ${FILE_CONF}
else
        echo "file ${FILE_CONF} not exists"
        exit 1
fi
### generated variables
TAP_NAME=tap${GUEST_ID}
MAC_ADDR=00:16:3e:${GUEST_ID}:00:01
VNC_DISPLAY=3${GUEST_ID}00
VNC_PORT=`expr ${VNC_DISPLAY} + 5900`
GUEST_IP=10.18.18.1${GUEST_ID}
### options for kvm
OPT_BOOT="-boot c"
OPT_CDROM=""
OPT_STD_VGA="-std-vga"
OPT_USBDEVICE="-usbdevice tablet"
OPT_NO_ACPI=""
OPT_CPU="-cpu qemu64"
OPT_NIC="-net nic,macaddr=${MAC_ADDR},model=rtl8139"
OPT_DRIVE=""
OPT_HDB=""
OPT_VNC="-vnc :${VNC_DISPLAY}"
OPT_HDA="-hda ${FILE_HDA}"
OPT_SMP="-smp 4"
OPT_SERIAL=""
if [ -f ${FILE_OVERRIDE} ]
then
        . ${FILE_OVERRIDE}
fi
### network
start_net() {
        echo "start network adapter ${TAP_NAME}"
        sudo tunctl -u twer -t ${TAP_NAME}
        sudo ifconfig ${TAP_NAME} 0.0.0.0 promisc up
        sudo brctl addif br0 ${TAP_NAME}
}
stop_net() {
        echo "stop network adapter ${TAP_NAME}"
        sudo tunctl -d ${TAP_NAME}
}
check_net_status() {
        NET_STATUS=`ifconfig | grep ${TAP_NAME}`
        if test "${NET_STATUS}" = ""
        then
                echo "network adapter ${TAP_NAME} has not been started"
        else
                echo "network adapter ${TAP_NAME} has been started"
        fi
}
## virtual machine
start_vm_sliently() {
        echo "start virtual machine"
        sudo modprobe kvm
        sudo modprobe kvm_intel
        if ! [ -d /dev/km ]
        then
                sleep 1
        fi
        sudo chmod 666 /dev/kvm
        sudo chmod 666 /dev/net/tun
        if ! [ -d ${DIR_RUN} ]
        then
                mkdir ${DIR_RUN}
        fi
        qemu-system-x86_64 \
                ${OPT_HDA} \
                ${OPT_HDB} \
                ${OPT_DRIVE} \
                ${OPT_CPU} \
                ${OPT_SMP} \
                ${OPT_CDROM} \
                -m ${GUEST_MEMORY} \
                ${OPT_BOOT} \
                ${OPT_USBDEVICE} \
                ${OPT_NIC} \
                ${OPT_SERIAL} \
                -net tap,ifname=${TAP_NAME} \
                -k en-us \
                ${OPT_STD_VGA} \
                -monitor unix:${FILE_MONITOR},server,nowait \
                -pidfile ${FILE_PID} \
                ${OPT_NO_ACPI} \
                ${OPT_VNC} &
        # check if the pid file created successfully
        if [ ! -f ${FILE_PID} ]
        then
                sleep 1
        fi
        if [ ! -f ${FILE_PID} ]
        then
                return 1
        fi
        # check if the process started successfully
        if [ ! -d /proc/`cat ${FILE_PID}` ]
        then
                return 1
        fi
}
start_vm() {
        start_vm_sliently

        # if start_vm_sliently return -1
        if test $? -eq -1
        then
                echo "startup failed. check ${FILE_OUT}"
                exit 1
        else
                echo "startup successfully"
        fi
}
send_cmd() {
        QEMU_MONITOR_COMMAND=$1
        echo "${QEMU_MONITOR_COMMAND}" | socat - UNIX-CONNECT:${FILE_MONITOR}
}
get_vm_pid_to() {
        ACTION_TO_DO=$1
        # check if pid file there
        if [ ! -f ${FILE_PID} ]
        then
                echo "${FILE_PID} not found, can not ${ACTION_TO_DO}"
                exit 1
        fi
        VM_PID=`cat ${FILE_PID}`
}
check_vm_status() {
        get_vm_pid_to "check vm status"
        if [ -d /proc/${VM_PID} ]
        then
                echo "vm is running at process id ${VM_PID}"
        else
                echo "vm is not running"
        fi
}
stop_vm() {
        echo "stop virtual machine"

        get_vm_pid_to "stop vm"
        # check if monitor file there
        if [ ! -e ${FILE_MONITOR} ]
        then
                echo "${FILE_MONITOR} not found, can not stop vm"
                exit 1
        fi
        # if the process is still running
        # send command quit to its monitor, and wait
        if [ -d /proc/${VM_PID} ]
        then
                send_cmd "quit"
        fi
        # check if the process is still running
        if [ -d /proc/${VM_PID} ]
        then
                sleep 1
        fi
        if [ ! -d /proc/${VM_PID} ]
        then
                # yes, done
                rm ${FILE_PID}
                rm ${FILE_MONITOR}
                echo "vm stopped successfully"
        else
                # no, something wrong there...
                echo "failed to stop vm"
                exit 1
        fi
}
kill_vm() {
        echo "kill virtual machine"

        get_vm_pid_to "kill vm"
        # if the process is still running, kill it
        if [ -d /proc/${VM_PID} ]
        then
                kill ${VM_PID}
        fi
        rm ${FILE_PID}
        rm ${FILE_MONITOR}
        echo "vm killed"
}
### Main switch
case "$1" in
start-net)
        start_net
        ;;
start-vm)
        start_vm
        ;;
start)
        start_net
        start_vm
        ;;
status)
        check_net_status
        check_vm_status
        ;;
cad)
        send_cmd "sendkey ctrl-alt-delete"
        ;;
vnc)
        vncviewer localhost:${VNC_PORT} &
        ;;
rdesktop)
        rdesktop $2 $3 ${GUEST_IP} &
        ;;
ssh)
        ssh ${GUEST_IP}
        ;;
ping)
        ping ${GUEST_IP}
        ;;
halt)
        ssh twer@${GUEST_IP} sudo halt
        ;;
reset)
        send_cmd "system_reset"
        ;;
stop-vm)
        stop_vm
        ;;
stop-net)
        stop_net
        ;;
stop)
        stop_vm
        stop_net
        ;;
kill)
        kill_vm
        sleep 1
        stop_net
        ;;
*)
        echo "You need to specify a action, available actions are:"
        echo "[start-net] start network of the virtual machine"
        echo "[start-vm] start virtual machine itself"
        echo "[start] start both"
        echo "[status] check the status of network and virtual machine"
        echo "[cad] ctrl-alt-delete"
        echo "[vnc] use vinagre to view the vnc of the guest"
        echo "[rdesktop] remote desktop to the guest"
        echo "[ssh] ssh to the guest"
        echo "[ping] ping guest"
        echo "[halt] ssh to the guest and halt the guest"
        echo "[reset] reset the virtual machine"
        echo "[stop-vm] power off vritual machine"
        echo "[stop-net] stop network of the virtual machine"
        echo "[stop] stop both"
        echo "[kill] kill the viritual machine and network"
        exit 1
        ;;
esac
exit
