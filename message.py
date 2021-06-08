#---------------------------------------------------------------------------------------------------------------------
#--------------------------------------------------- Remote Pis-----------------------------------------------------------
#---------------------------------------------------------------------------------------------------------------------

import netifaces as ni


remote_ip = [
    #1                  2                   3               4
    "169.254.242.48", "169.254.62.239","169.254.153.125","169.254.197.148",
    #5                  6                  7                8
    "169.254.231.54", "169.254.144.17","169.254.194.207","169.254.156.201",
    #9                  10                  11              12
    "169.254.103.239","169.254.242.146","169.254.1.137","169.254.119.185",
    #13                 14                  15              16
    "169.254.155.175","169.254.17.33","169.254.154.203","169.254.80.5",
    #17                 18                  19              20
    "169.254.47.35","169.254.54.153","169.254.206.169","169.254.48.133",
    #21
    "169.254.232.253"
]

@parallel
@hosts(remote_ip)
def test_remote():
    run("hostname")

remote_tree = {
    remote_ip[0] : "'"+remote_ip[15]+";"+remote_ip[5]+";"+remote_ip[20]+";"+remote_ip[1]+"'",
    remote_ip[1] : "''",
    remote_ip[2] : "'"+remote_ip[3]+"'",
    remote_ip[3] : "'"+remote_ip[6]+"'",
    remote_ip[4] : "'"+remote_ip[7]+"'",
    remote_ip[5] : "'"+remote_ip[17]+";"+remote_ip[18]+"'",
    remote_ip[6] : "'"+remote_ip[14]+"'",
    remote_ip[7] : "'"+remote_ip[8]+";"+remote_ip[19]+"'",
    remote_ip[8] : "''",
    remote_ip[9] : "'"+remote_ip[4]+";"+remote_ip[11] +"'",
    remote_ip[10] : "''",
    remote_ip[11] : "'"+remote_ip[10]+"'",
    remote_ip[12] : "''",
    remote_ip[13] : "'"+remote_ip[12]+";"+remote_ip[16]+"'",
    remote_ip[14] : "'"+remote_ip[13]+"'",
    remote_ip[15] : "'"+remote_ip[2]+"'",
    remote_ip[16] : "''",
    remote_ip[17] : "''",
    remote_ip[18] : "'"+remote_ip[9]+"'",
    remote_ip[19] : "''",
    remote_ip[20] : "''"
}

env.password="12345"


def get_hosts(ip):
    #print ip + " : " + remote_tree[ip]
    return remote_tree[ip]

def execute_remote(serv_command='', command=''):
    run("fab execute:"+serv_command+","+command)


def execute(serv_command='', command=''):
    myIP=str(ni.ifaddresses('wlan0')[ni.AF_INET][0]['addr'])
    __hosts=get_hosts(myIP)
    if serv_command=='':
        local(command)
    if __hosts != "''":
        print("["+myIP+"]: fab chain_exec:"+command+",hosts=" + str(__hosts))
        local("fab chain_exec:"+command+",hosts=" + str(__hosts))
    if serv_command!='':
        local(serv_command)



@parallel
def chain_exec(command=''):
    #run(command)
    myIP=str(ni.ifaddresses('wlan0')[ni.AF_INET][0]['addr'])
    __hosts = get_hosts(myIP)
    if __hosts != "''":
        print("["+myIP+"]: fab execute:command="+str(command))
        run("fab execute:command="+str(command))




def f_execute_program(serv_name='', cl_name='', e_run='', final=''):
    myIP=str(ni.ifaddresses('wlan0')[ni.AF_INET][0]['addr'])
    __hosts=get_hosts(myIP)

    if serv_name == '':
        local("/usr/bin/nohup sudo ./execute_program " + str(cl_name) + " " + str(e_run))


    if __hosts != "''":
        print("[DEBUG]["+myIP+"]: fab chain_exec_program:"+cl_name+","+e_run+",hosts="+str(__hosts))
        local("fab chain_exec_program:"+cl_name+","+e_run+",hosts="+str(__hosts))


    if serv_name != '':
        local("/usr/bin/nohup sudo ./execute_program "+ str(serv_name) + " " + str(cl_name)+str(e_run))
        local(final)
    #else
    #    local("/usr/bin/nohup sudo ./execute_program " + cl_name + " " + e_run + "")


@parallel
def chain_exec_program(e_name='', e_run=''):
    #run("/usr/bin/nohup sudo ./execute_program " + e_name + " " + e_run + "")
    myIP=str(ni.ifaddresses('wlan0')[ni.AF_INET][0]['addr'])
    #__hosts=get_hosts(myIP)
    #if __hosts != "''":
    print("[DEBUG]["+myIP+"]: fab f_execute_program:cl_name="+e_name+",e_run="+e_run)#",hosts="+str(__hosts)
    run("fab f_execute_program:cl_name="+e_name+",e_run="+e_run)#+",hosts="+str(__hosts))




def stop_program():
    myIP=str(ni.ifaddresses('wlan0')[ni.AF_INET][0]['addr'])
    __hosts=get_hosts(myIP)

    if __hosts != "''":
        print("["+myIP+"]: fab chain_stop_program,hosts="+str(__hosts))
        local("fab chain_stop_program:hosts="+str(__hosts))

    local("touch control.file")


@parallel
def chain_stop_program():
    myIP=str(ni.ifaddresses('wlan0')[ni.AF_INET][0]['addr'])
    #__hosts=get_hosts(myIP)
    #if __hosts != "''":
    print("["+myIP+"]: fab stop_program")#+",hosts="+str(__hosts)
    run("fab stop_program")#+",hosts="+str(__hosts))
    #print "sudo killall -9 " + str(e_name)
    #run("sudo killall -9 " + str(e_name))




def kill_program(serv_name='', cl_name=''):
    myIP=str(ni.ifaddresses('wlan0')[ni.AF_INET][0]['addr'])
    __hosts=get_hosts(myIP)

    if __hosts != "''":
        print( "["+myIP+"]: fab chain_kill_program:e_name="+cl_name+",hosts="+str(__hosts))
        local("fab chain_kill_program:e_name="+cl_name+",hosts="+str(__hosts))
    if serv_name != '':
        print("sudo killall -9 "+ str(serv_name))
        with warn_only():
            local("sudo killall -9 "+ str(serv_name))
    else:
        print("sudo killall -9 " + str(cl_name))
        with warn_only():
            local("sudo killall -9 " + str(cl_name))


@parallel
def chain_kill_program(e_name=''):
    myIP=str(ni.ifaddresses('wlan0')[ni.AF_INET][0]['addr'])
    #__hosts=get_hosts(myIP)
    #if __hosts != "''":
    print("["+myIP+"]: fab kill_program:cl_name="+str(e_name))#+",hosts="+str(__hosts)
    run("fab kill_program:cl_name="+e_name)#+",hosts="+str(__hosts))
    #print "sudo killall -9 " + str(e_name)
    #run("sudo killall -9 " + str(e_name))


def rm_file(file_name=''):
    myIP=str(ni.ifaddresses('wlan0')[ni.AF_INET][0]['addr'])
    __hosts=get_hosts(myIP)

    if __hosts != "''":
        print("["+myIP+"]: fab chain_rm_file:file_name="+file_name+",hosts="+str(__hosts))
        local("fab chain_rm_file:file_name="+file_name+",hosts="+str(__hosts))

    print("sudo rm " + str(file_name))
    with warn_only():
        local("sudo rm " + str(file_name))


@parallel
def chain_rm_file(file_name=''):
    myIP=str(ni.ifaddresses('wlan0')[ni.AF_INET][0]['addr'])
    #__hosts=get_hosts(myIP)
    #if __hosts != "''":
    print("["+myIP+"]: fab rm_file:file_name="+str(file_name))#+",hosts="+str(__hosts)
    run("fab rm_file:file_name="+file_name)#+",hosts="+str(__hosts))
    #print "sudo killall -9 " + str(e_name)
    #run("sudo killall -9 " + str(e_name))

remote_ygg_lowlevel_home= "./Production/Yggdrasil-LowLevelLib"
remote_yggdrasil_home= "./Production/Yggdrasil"

@parallel
@hosts(remote_ip)
def remote_sync():
    local('rsync -avz --delete --exclude "*CMakeFiles*" --exclude "*.o" --rsh="ssh -o StrictHostKeyChecking=no" -i ' + env.userKey + ' ' + remote_ygg_lowlevel_home + ' ' + env.user + '@%s:~/Production/' % (env.host))
    local('rsync -avz --delete --exclude "*CMakeFiles*" --exclude "*.o" --rsh="ssh -o StrictHostKeyChecking=no" -i ' + env.userKey + ' ' + remote_yggdrasil_home + ' ' + env.user + '@%s:~/Production/' % (env.host))
    local('rsync -avz --rsh="ssh -o StrictHostKeyChecking=no" -i ' + env.userKey + ' Production/CMakeLists.txt ' + env.user + '@%s:~/Production/' % (env.host))
    local('rsync -avz --rsh="ssh -o StrictHostKeyChecking=no" -i ' + env.userKey + ' Production/bin/* ' + env.user + '@%s:~/Production/bin/' % (env.host))
    local('rsync -avz --rsh="ssh -o StrictHostKeyChecking=no" -i ' + env.userKey + ' fabfile.py ' + env.user + '@%s:~/' % (env.host))
    local('rsync -avz --rsh="ssh -o StrictHostKeyChecking=no" -i ' + env.userKey + ' tools ' + env.user + '@%s:~/' % (env.host))


def remote_update():
    myIP=str(ni.ifaddresses('wlan0')[ni.AF_INET][0]['addr'])
    __hosts=get_hosts(myIP)


    if __hosts != "''":
        local("fab remote_sync:hosts="+str(__hosts))
        local("fab chain_remote_update:hosts="+str(__hosts))



@parallel
def chain_remote_update():
    run("fab remote_update")

def remote_boot_config():
    myIP=str(ni.ifaddresses('wlan0')[ni.AF_INET][0]['addr'])
    __hosts=get_hosts(myIP)


    if __hosts != "''":
        local("fab configureboot:hosts="+str(__hosts))
        local("fab chain_remote_boot:hosts="+str(__hosts))


@parallel
def chain_remote_boot():
    run("fab remote_boot_config")



def remote_build(target):
    myIP=str(ni.ifaddresses('wlan0')[ni.AF_INET][0]['addr'])
    __hosts=get_hosts(myIP)

    if __hosts != "''":
        local("fab build:"+target+",hosts="+str(__hosts))
        local("fab chain_remote_build:"+target+",hosts="+str(__hosts))




@parallel
def chain_remote_build(target):
    run("fab remote_build:"+target)


def mv_file(file_name='',dest=''):
    myIP=str(ni.ifaddresses('wlan0')[ni.AF_INET][0]['addr'])
    __hosts=get_hosts(myIP)

    if __hosts != "''":
        print("["+myIP+"]: fab chain_mv_file:file_name="+file_name+",hosts="+str(__hosts))
        local("fab chain_mv_file:file_name="+file_name+",dest="+dest+",hosts="+str(__hosts))

    print("sudo mv " + str(file_name) +" "+ str(dest))
    with warn_only():
        local("sudo mv " + str(file_name) +" "+ str(dest))


@parallel
def chain_mv_file(file_name='',dest=''):
    myIP=str(ni.ifaddresses('wlan0')[ni.AF_INET][0]['addr'])
    #__hosts=get_hosts(myIP)
    #if __hosts != "''":
    print("["+myIP+"]: fab mv_file:file_name="+str(file_name))#+",hosts="+str(__hosts)
    run("fab mv_file:file_name="+file_name+",dest="+dest)#+",hosts="+str(__hosts))
    #print "sudo killall -9 " + str(e_name)
    #run("sudo killall -9 " + str(e_name))


def mk_dir(file_name=''):
    myIP=str(ni.ifaddresses('wlan0')[ni.AF_INET][0]['addr'])
    __hosts=get_hosts(myIP)

    if __hosts != "''":
        print("["+myIP+"]: fab chain_rm_file:file_name="+file_name+",hosts="+str(__hosts))
        local("fab chain_mk_dir:file_name="+file_name+",hosts="+str(__hosts))

    print("sudo rm " + str(file_name))
    local("mkdir " + str(file_name))


@parallel
def chain_mk_dir(file_name=''):
    myIP=str(ni.ifaddresses('wlan0')[ni.AF_INET][0]['addr'])
    #__hosts=get_hosts(myIP)
    #if __hosts != "''":
    print("["+myIP+"]: fab rm_file:file_name="+str(file_name))#+",hosts="+str(__hosts)
    run("fab mk_dir:file_name="+file_name)#+",hosts="+str(__hosts))
    #print "sudo killall -9 " + str(e_name)
    #run("sudo killall -9 " + str(e_name))


@parallel
@hosts(remote_ip)
def remote_get(dir):
    get(dir)

@parallel
@hosts(raspis)
def local_get(dir):
    get(dir)


def remote_reboot():
    myIP=str(ni.ifaddresses('wlan0')[ni.AF_INET][0]['addr'])
    __hosts=get_hosts(myIP)
    if __hosts != "''":
        local("fab chain_reboot:hosts="+str(__hosts))

    local("sudo reboot")

@parallel
def chain_reboot():
    with warn_only():
        run("fab remote_reboot")


def remote_shutdown():
    myIP=str(ni.ifaddresses('wlan0')[ni.AF_INET][0]['addr'])
    __hosts=get_hosts(myIP)
    if __hosts != "''":
        local("fab chain_shutdown:hosts="+str(__hosts))

    local("sudo shutdown")

@parallel
def chain_shutdown():
    with warn_only():
        run("fab remote_shutdown")
