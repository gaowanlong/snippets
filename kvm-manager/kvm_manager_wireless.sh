#!/bin/bash

######################
## Default settings ##
######################

## Directory and files
if ! [ -d $2 ]
then
	DIR_BASE=`pwd $`
else
	DIR_BASE=$2
fi
FILE_HDA=${DIR_BASE}/hda
FILE_CONF=${DIR_BASE}/conf

### run directory will be auto-created with following files
DIR_RUN=${DIR_BASE}/run
FILE_MONITOR=${DIR_RUN}/monitor
FILE_PID=${DIR_RUN}/pid
FILE_OUT=${DIR_RUN}/out

GUEST_ID=0
GUEST_MEMORY=1024
GUEST_IP=192.168.1.97
HOST_IP=192.168.1.90/24
HOST_INTERFACE=wlan0

### generated variables
TAP_NAME=tap${GUEST_ID}
#MAC_ADDR=00:16:3e:${GUEST_ID}:00:01
#VNC_DISPLAY=3${GUEST_ID}00
#VNC_PORT=`expr ${VNC_DISPLAY} + 5900`

### options for kvm
#OPT_BOOT="-boot c"
OPT_BOOT=""

#OPT_CDROM="-cdrom /media/cdrom"
OPT_CDROM=""

#OPT_STD_VGA="-std-vga"
OPT_STD_VGA=""

#OPT_USBDEVICE="-usbdevice tablet"
OPT_USBDEVICE=""

#OPT_NO_ACPI="-no-acpi"
OPT_NO_ACPI=""

#OPT_CPU="-cpu qemu64"
OPT_CPU=""

#OPT_NIC="-net nic,macaddr=${MAC_ADDR},model=rtl8139"
OPT_NIC="-net nic"

#OPT_DRIVE="-drive <drive>"
OPT_DRIVE=""

#OPT_HDB="-hdb <hdb>"
OPT_HDB=""

#OPT_VNC="-vnc :${VNC_DISPLAY}"
OPT_VNC=""

#OPT_HDA="-hda <hda>"
OPT_HDA="-hda ${FILE_HDA}"

#OPT_SMP="-smp 4"
OPT_SMP=""

OPT_SERIAL=""

OPT_OTHER=""

if [ -f ${FILE_CONF} ]
then
        . ${FILE_CONF}
fi

### wireless network
start_net() {
        echo "start network adapter ${TAP_NAME}"
        sysctl net.ipv4.ip_forward=1
        tunctl -b -t ${TAP_NAME} -u `whoami`
        ip link set ${TAP_NAME} up
        ip addr add ${HOST_IP} dev ${TAP_NAME}
        parprouted ${HOST_INTERFACE}  ${TAP_NAME}
        echo "network adapter ${TAP_NAME} is started with as ip ${GUEST_IP}"
}
stop_net() {
        echo "stop network adapter ${TAP_NAME}"
        tunctl -d ${TAP_NAME}
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
        
        if ! [ -d ${DIR_RUN} ]
        then
                mkdir ${DIR_RUN}
        fi

        kvm \
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
                -net tap,ifname=${TAP_NAME},script=no \
                -k fr \
                ${OPT_STD_VGA} \
                -monitor unix:${FILE_MONITOR},server,nowait \
                -pidfile ${FILE_PID} \
                ${OPT_NO_ACPI} \
                ${OPT_VNC} \
                ${OPT_OTHER} &

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
detect_module() {
. /lib/lsb/init-functions

# Figure out which module we need.
	if grep -q ^flags.*\\\<vmx\\\> /proc/cpuinfo
	then
		module=kvm_intel
	elif grep -q ^flags.*\\\<svm\\\> /proc/cpuinfo
	then
		module=kvm_amd
	else
		module=
	fi
}
start_kvm() {
	detect_module

	if [ -z "$module" ]
	then
		log_failure_msg "Your system does not have the CPU extensions required to use KVM. Not doing anything."
		exit 0
	fi
	if modprobe "$module" 
	then
		log_success_msg "Loading kvm module $module"
	else
		log_failure_msg "Module $module failed to load"
		exit 1
	fi
}
stop_kvm() {
	detect_module

	if [ -z "$module" ]
	then
		exit 0
	fi
	if lsmod | grep -q "$module"
	then
		if rmmod "$module" 
		then
			log_success_msg "Succesfully unloaded kvm module $module"
			rmmod kvm
		else
			log_failure_msg "Failed to remove $module"
			exit 1
		fi
	else
		log_failure_msg "Module $module not loaded"
	fi
}
### Main switch
case "$1" in

start-kvm)
	start_kvm
	;;
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
        ssh root@${GUEST_IP} halt
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
stop-kvm)
	stop_kvm
	;;
kill)
        kill_vm
        sleep 1
        stop_net
        ;;

*)
	echo "KVM manager version 0.1, Samuel Bally"
	echo "usage: kvm-manager [options] [path]"
	echo ""
	echo "Standard options:"      
	echo "You need to specify a action, available actions are:"
	echo "[start-kvm] start kvm module"
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
	echo "[stop-kvm] stop kvm module"
        echo "[stop] stop both"
        echo "[kill] kill the viritual machine and network"
	echo ""
	echo "Config file:"
	echo "create a conf file in your vm directory for change default settings"
	echo "conf varialbles:"
	echo "DIR_RUN"
	echo "FILE_MONITOR"
	echo "FILE_PID"
	echo "FILE_OUT"
	echo "GUEST_ID"
	echo "GUEST_MEMORY"
	echo "GUEST_IP"
	echo "HOST_IP"
	echo "HOST_INTERFACE"
	echo "OPT_BOOT"
	echo "OPT_CDROM"
	echo "OPT_STD_VGA"
	echo "OPT_USBDEVICE"
	echo "OPT_NO_ACPI"
	echo "OPT_CPU"
	echo "OPT_NIC"
	echo "OPT_DRIVE"
	echo "OPT_HDB"
	echo "OPT_VNC"
	echo "OPT_HDA"
	echo "OPT_SMP"
	echo "OPT_SERIAL"
	echo "OPT_OTHER"
        exit 1
        ;;

esac
exit
