import os
import getpass
import datetime
from fabric.api import *

#lightkone_home="./Yggdrasil/Yggdrasil-LowLevelLib"
lightkone_home="./Yggdrasil"
lightkone_protos_home="./Yggdrasil/Yggdrasil"
com_prim_home="./CommunicationPrimitives"

#lightkone_remote="~/Yggdrasil/Yggdrasil-LowLevelLib"
lightkone_remote="~/Yggdrasil"
lightkone_protos_remote="~/Yggdrasil/Yggdrasil"
com_prim_remote="~/CommunicationPrimitives"


#raspis = [
#    '192.168.1.101','192.168.1.102','192.168.1.103','192.168.1.104','192.168.1.105',
#    '192.168.1.106','192.168.1.107','192.168.1.108','192.168.1.109','192.168.1.110',
#    '192.168.1.111','192.168.1.112','192.168.1.113','192.168.1.114','192.168.1.115',
#    '192.168.1.116','192.168.1.117','192.168.1.118','192.168.1.119','192.168.1.120'
#    ]
    
raspis = [
    '192.168.1.101','192.168.1.102','192.168.1.103','192.168.1.105',
    '192.168.1.106','192.168.1.107','192.168.1.108','192.168.1.109','192.168.1.110',
    '192.168.1.111','192.168.1.112','192.168.1.113','192.168.1.114','192.168.1.115',
    '192.168.1.116','192.168.1.117','192.168.1.118','192.168.1.119'
    ]


def __get_hosts(hosts_dict, hosts_per_dc):
    hosts = []
    hosts = raspis
    return hosts


env.user = getpass.getuser()
env.disable_known_hosts = True
env.userKey = '~/.ssh/id_rsa'

@parallel
@hosts(raspis)
def setethers():
    run('sudo cp ~/tools/ethers /etc/')

@parallel
@hosts(raspis)
def setarptable():
    run('sudo arp -f')

@parallel
@hosts(raspis)
def clock():
    run('date');

@parallel
@hosts(raspis)
def synclock():
#    env.date = local('date')
    now = datetime.datetime.now()
    local('ssh -o StrictHostKeyChecking=no -i ' + env.userKey + ' ' + env.user + '@' + env.host + ' "sudo date +\'%Y-%m-%d %T.%N\' -s \'' + str(now) + '\'"')


@parallel
@hosts(raspis)
def sync():

    local('rsync -avz --delete --rsync-path="mkdir -p ' + lightkone_remote + ' && rsync"  --exclude "*CMakeFiles*" --exclude "*.o" --rsh="ssh -o StrictHostKeyChecking=no" -i ' + env.userKey + ' ' + lightkone_home + ' ' + env.user + '@%s:~/' % (env.host))

#    local('rsync -avz  --delete --rsync-path="mkdir -p ' + lightkone_protos_remote + ' && rsync" --exclude "*CMakeFiles*" --exclude "*.o" --rsh="ssh -o StrictHostKeyChecking=no" -i ' + env.userKey + ' ' + lightkone_protos_home + ' ' + env.user + '@%s:%s' % (env.host, lightkone_protos_remote))

    local('rsync -avz  --delete --rsync-path="mkdir -p ' + com_prim_remote + ' && rsync" --exclude "*CMakeFiles*" --exclude "*.o" --rsh="ssh -o StrictHostKeyChecking=no" -i ' + env.userKey + ' ' + com_prim_home + ' ' + env.user + '@%s:~/' % (env.host))

    local('rsync -avz --rsh="ssh -o StrictHostKeyChecking=no" -i ' + env.userKey + ' CMakeLists.txt ' + env.user + '@%s:~/' % (env.host))
    #local('rsync -avz --rsh="ssh -o StrictHostKeyChecking=no" -i ' + env.userKey + ' tools ' + env.user + '@%s:~/' % (env.host))

    print(com_prim_home)
    print(com_prim_remote)


@parallel
@hosts(raspis)
def createLighkoneConfigDir():
    run("sudo mkdir /etc/lightkone | 0; sudo mkdir /etc/lightkone/topologyControl | 0; sudo chmod -R 777 /etc/lightkone")

@parallel
@hosts(raspis)
def createYggdrasilHome():
    run("sudo mkdir /home/yggdrasil; sudo chmod -R 755 /home/yggdrasil")

@parallel
@hosts(raspis)
def configureYggdrasilHome():
    run("sudo mkdir /home/yggdrasil/bcast; sudo mkdir /home/yggdrasil/cmd; sudo mkdir /home/yggdrasil/bin")


@parallel
@hosts(raspis)
def updateYggdrasilControl():
    run("sudo cp ~/bin/YggdrasilControlProcess /home/yggdrasil/; sudo chmod 777 /home/yggdrasil/YggdrasilControlProcess")

@parallel
@hosts(raspis)
def addBinToYggdrasilHome():
    run("sudo cp /home/"+ (env.user) +"/bin/cmd* /home/yggdrasil/cmd; sudo cp /home/"+ (env.user) +"/bin/bcast_generic_test /home/yggdrasil/bin")
    run("sudo chmod 777 /home/yggdrasil/cmd/*; sudo chmod 777 /home/yggdrasil/bin/*")

@parallel
@hosts(raspis)
def cleanYggdrasilHome():
    run("sudo rm -rf /home/yggdrasil/*")

@hosts(raspis)
def checkInforumLogs():
    run("ls -aloh /home/yggdrasil/inforum18")

@hosts(raspis)
def getInforumLogs():
    get("/home/yggdrasil/inforum18/*")

@hosts(raspis)
def getBcastLogs():
    get("/home/yggdrasil/bcast/*")

@hosts(raspis)
def backupInforumLogs():
    run("sudo mv /home/yggdrasil/inforum18 /home/yggdrasil/inforum18-remote1")
    run("sudo mkdir /home/yggdrasil/inforum18")

@parallel
@hosts(raspis)
def updateTopology():
    local('rsync -avz --rsh="ssh -o StrictHostKeyChecking=no" -i ' + env.userKey + ' topologyControl/macAddrDB.txt ' + env.user + '@%s:/etc/lightkone/topologyControl/' % (env.host))
    local('scp -i ' + env.userKey + ' topologyControl/'+(env.host)+ ' ' + env.user + '@%s:/etc/lightkone/topologyControl/neighs.txt' % (env.host))
    run("sudo chmod -R 777 /etc/lightkone")

@parallel
@hosts(raspis)
def removeTopology():
    run("sudo rm /etc/lightkone/topologyControl/neighs.txt")

@parallel
@hosts(raspis)
def test():
    run("hostname")


@parallel
@hosts(raspis)
def reboot():
    run("sudo reboot")


@parallel
@hosts(raspis)
def shutdown():
    run("sudo shutdown now")

@parallel
@hosts(raspis)
def configureboot():
    with cd("~/tools/"):
        run("sudo mv lightkone.sh /etc/init.d/")
        run("sudo chmod +x /etc/init.d/lightkone.sh")
        run("sudo cp *.sh /home/yggdrasil/")
        run("sudo chmod +x /home/yggdrasil/*.sh")
        run("sudo cp runonboot.txt /home/yggdrasil/")
    with cd("/etc/init.d"):
        run("sudo update-rc.d lightkone.sh defaults")

@parallel
@hosts(raspis)
def build():
    run("cmake .; make")

@parallel
@hosts(raspis)
def rebuild():
    run("make")

@parallel
@hosts(raspis)
def runtest(test):
    with cd("~/tools/"):
        run("/usr/bin/nohup ./run.sh " + test)

@parallel
@hosts(raspis)
def runtestwitharg(test, arg):
    with cd("~/tools/"):
        run("/usr/bin/nohup ./runWithArg.sh " + test + " " + arg)

@parallel
@hosts(raspis)
def checktestlive(test):
    run("sudo ps ax | grep '"+test+"' | wc -l")

@parallel
@hosts(raspis)
def stopcontrol():
    run("sudo killall -3 YggdrasilControlProcess")

@parallel
@hosts(raspis)
def stoptest(test):
    run("sudo killall -3 " + test)

@parallel
@hosts(raspis)
def killtest(test):
    run("sudo killall -9 " + test)

@parallel
@hosts(raspis)
def armageddon():
    run("sudo rm -rf ~/*")

@parallel
@hosts(raspis)
def rmnohup():
    run("sudo rm tools/nohup.out")

@parallel
@hosts(raspis)
def getBcastResults():
    run("sudo scp -o StrictHostKeyChecking=no -i " + env.userKey + " -r 192.168.137.111:/home/andreffrosa/results/ .")
    run("sudo scp -o StrictHostKeyChecking=no -i " + env.userKey + " -r 192.168.137.112:/home/andreffrosa/results/ .")
    run("sudo scp -o StrictHostKeyChecking=no -i " + env.userKey + " -r 192.168.137.123:/home/andreffrosa/results/ .")
    run("sudo scp -o StrictHostKeyChecking=no -i " + env.userKey + " -r 192.168.137.120:/home/andreffrosa/results/ .")
    run("sudo scp -o StrictHostKeyChecking=no -i " + env.userKey + " -r 192.168.137.119:/home/andreffrosa/results/ .")


@parallel
@hosts(raspis)
def runBcastTests1():
    run("sudo ./bin/bcast_test -r -2 -t 0 -c 0 -i 5000 -s 1000 -b -p 0.5 -d 60 >> ./results/flooding+nullTimer.txt")


@parallel
@hosts(raspis)
def cleanHome():
    run("sudo rm -rf /home/andreffrosa/*")

@parallel
@hosts(raspis)
def createBcastFolder():
    run("sudo mkdir /home/yggdrasil/bcast_results/")



@parallel
@hosts(raspis)
def uploadTopology(src, dest):
    if not src.endswith("/"):
        src = src + "/"

    if not dest.endswith("/"):
        dest = dest + "/"

    if(os.path.exists(src+env.host)):
        local('rsync -avz --rsync-path="mkdir -p ' + dest + ' && touch ' + dest + 'macAddrDB.txt  && rsync" --rsh="ssh -o StrictHostKeyChecking=no" -i ' + env.userKey + ' ' + src + 'macAddrDB.txt ' + env.user + '@' + env.host + ':' + dest)

        local('scp -i ' + env.userKey + ' ' + src+env.host + ' ' + env.user + '@'+env.host+':' + dest + 'neighs.txt')
        run("sudo chmod -R 777 " + dest)

@parallel
@hosts(raspis)
def uploadTopologies(src="./topologies/", dest="~/topologies/"):
    for o in os.listdir(src):
        temp_src = os.path.join(src, o)
        if os.path.isdir(temp_src):
            temp_dest = os.path.join(src, o)
            uploadTopology(temp_src, temp_dest)

@parallel
@hosts(raspis)
def uploadConfigs(src="./experiments/", dest="~/"):
    if not src.endswith("/"):
        src = src + "/"

    if not dest.endswith("/"):
        dest = dest + "/"

    local('scp -i ' + env.userKey + ' -r ' + src + ' ' + env.user + '@'+env.host+':' + dest)
    run("sudo chmod -R 777 " + dest)

@parallel
@hosts(raspis)
def uploadProject(src="./", dest="~/"):
    if not src.endswith("/"):
        src = src + "/"

    if not dest.endswith("/"):
        dest = dest + "/"

    # CommunicationPrimitives
    local('scp -i ' + env.userKey + ' -r ' + src+"CommunicationPrimitives/" + ' ' + env.user + '@'+env.host+':' + dest+"CommunicationPrimitives/")
    run("sudo chmod -R 777 " + dest+"CommunicationPrimitives/")

    # experiments
    local('scp -i ' + env.userKey + ' -r ' + src+"experiments/configs/" + ' ' + env.user + '@'+env.host+':' + dest+"experiments/configs/")
    run("sudo chmod -R 777 " + dest+"experiments/configs/")

    # scripts
    local('scp -i ' + env.userKey + ' -r ' + src+"scripts/remote/" + ' ' + env.user + '@'+env.host+':' + dest+"scripts/")
    run("sudo chmod -R 777 " + dest+"scripts/")

    # topologies
    local('scp -i ' + env.userKey + ' -r ' + src+"topologies/" + ' ' + env.user + '@'+env.host+':' + dest+"topologies/")
    run("sudo chmod -R 777 " + dest+"topologies/")

    # Yggdrasil
    local('scp -i ' + env.userKey + ' -r ' + src+"Yggdrasil/" + ' ' + env.user + '@'+env.host+':' + dest+"Yggdrasil/")
    run("sudo chmod -R 777 " + dest+"Yggdrasil/")

    # CMakeLists.txt
    local('scp -i ' + env.userKey + ' -r ' + src+"CMakeLists.txt" + ' ' + env.user + '@'+env.host+':' + dest+"CMakeLists.txt")
    run("sudo chmod -R 777 " + dest+"CMakeLists.txt")


@parallel
@hosts(raspis)
def deploy(only_scripts=False):

    src="./"
    dest="~/"

    # sync clocks
    #now = datetime.datetime.now()
    #local('ssh -o StrictHostKeyChecking=no -i ' + env.userKey + ' ' + env.user + '@' + env.host + ' "sudo date +\'%Y-%m-%d %T.%N\' -s \'' + str(now) + '\'"')

    if not only_scripts:
        contents = ["CommunicationPrimitives", "experiments", "topologies", "Yggdrasil"]

        for x in contents:
            local('rsync -avz --delete --rsync-path="mkdir -p ' + dest+x + ' && rsync"  --exclude "*CMakeFiles*" --exclude "*.o" --rsh="ssh -o StrictHostKeyChecking=no" -i ' + env.userKey + ' ' + src+x + ' ' + env.user + '@%s:~/' % (env.host))

        local('rsync -avz --rsh="ssh -o StrictHostKeyChecking=no" -i ' + env.userKey + ' CMakeLists.txt ' + env.user + '@%s:~/' % (env.host))

    #scripts
    try:
        run("sudo rm -r " + dest + "scripts/")
    except:
        None
    local('scp -i ' + env.userKey + ' -r ' + src+"scripts/remote/" + ' ' + env.user + '@'+env.host+':' + dest+"scripts/")
    run("chmod -R 777 " + dest+"scripts/")

    if not only_scripts:
        run("cmake .; make")

@parallel
@hosts(raspis)
def deployConfigs():

    src="./"
    dest="~/"

    # sync clocks
    #now = datetime.datetime.now()
    #local('ssh -o StrictHostKeyChecking=no -i ' + env.userKey + ' ' + env.user + '@' + env.host + ' "sudo date +\'%Y-%m-%d %T.%N\' -s \'' + str(now) + '\'"')

    contents = ["experiments", "topologies"]

    for x in contents:
    	local('rsync -avz --delete --rsync-path="mkdir -p ' + dest+x + ' && rsync"  --exclude "*CMakeFiles*" --exclude "*.o" --rsh="ssh -o StrictHostKeyChecking=no" -i ' + env.userKey + ' ' + src+x + ' ' + env.user + '@%s:~/' % (env.host))

    	local('rsync -avz --rsh="ssh -o StrictHostKeyChecking=no" -i ' + env.userKey + ' CMakeLists.txt ' + env.user + '@%s:~/' % (env.host))

    #scripts
    try:
        run("sudo rm -r " + dest + "scripts/")
    except:
        None
    local('scp -i ' + env.userKey + ' -r ' + src+"scripts/remote/" + ' ' + env.user + '@'+env.host+':' + dest+"scripts/")
    run("sudo chmod -R 777 " + dest+"scripts/")



import pathlib

@parallel
@hosts(raspis)
def getExperinceResults(remote_dir,local_dir=None,unzip=False):

    dir1 = "/home/" + env.user + "/" + remote_dir + "/*"

    dir2 = (local_dir if local_dir != None else "./" + remote_dir + "/" )

    ip = env.host
    host = "raspi-"+ip[-2]+ip[-1]

    #file_path = dir2 + host + "/" +'%(basename)s' # zip
    file_path = dir2 + host + "/" +'%(path)s'

    #print(dir1)
    #print(dir2)
    #print(file_path)

    files = get(dir1, file_path)
    #print(files)

    if unzip:
        for f in files:
            path = pathlib.PurePath(f)

            if path.name.endswith(".tar.gz"):
                cmd = "tar -xf " + dir2 + host + "/" + path.name + " -C " + dir2 + host + "/"
                #print(cmd)
                local(cmd)
                local("rm " + dir2 + host + "/" + path.name)
                local("chmod 777 " + dir2 + host + "/" + path.name.replace(".tar.gz", "/"))

@parallel
@hosts(raspis)
def cleanResults(regex):
    run('sudo rm -r ' + regex )   

@parallel
@hosts(raspis)
def whoami():
    run('whoami')
    
    
    
@parallel
@hosts(raspis)
def network():
    run("sudo iw dev")
    
    
@parallel
@hosts(raspis)
def isrunning(regex):
    run('ps ax | grep "' + regex + '"')   
    

@parallel
@hosts(raspis)
def ip():
    run("ip a")   

@parallel
@hosts(raspis)
def arp():
    run("sudo cp /home/yggdrasil/tools/ethers /etc/")
    #run("sudo /home/yggdrasil/tools/")
    
    
@parallel
@hosts(raspis)  
def disable_avahi():
    run("sudo systemctl stop avahi-daemon.service ; sudo systemctl stop avahi-daemon.socket ; sudo systemctl disable avahi-daemon.service ; sudo systemctl disable avahi-daemon.socket")





