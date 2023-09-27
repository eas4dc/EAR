#!/usr/bin/env python3

import re

defaultPluginsList = ['eardbd.so', 'csv.so', 'csv_ts.so', 'dcdb.so', 'dcgmi.so', 'eard.so', 'examon.so', 'mpi_node_metrics.so', 'mpitrace.so', 'mysql.so', 'prometheus.so', 'psql.so', 'sysfs.so']

print("")
print("=========================== This tool is used to automatically build the EAR configuration file ===========================")
print("")
'''
print("============================= DB configuration =============================")
print("")
print("==== This configuration conrrespondons with the DB server installation. ====")
print("")

DBPluginType = input("Do you want to use a relational database plugins like mysql or postgres [Y/n]: ")
if DBPluginType == '':
    DBPluginType = 'y'
while not(DBPluginType in ['y', 'Y', 'n', 'N']) or DBPluginType.strip().isdigit():
    DBPluginType = input("[Y/n]: ")
    if DBPluginType == '':
        DBPluginType = 'y'

if DBPluginType in ['y', 'Y']:
    DBIp = input("Enter the IP address of the Database server [127.0.0.1]: ")
    if DBIp == '':
        DBIp = '127.0.0.1'
    while len(DBIp.split('.')) != 4:
        DBIp = input("Enter a valid IP address [127.0.0.1]: ")
        if DBIp == '':
            DBIp = '127.0.0.1'
    
    DBPort = input("Enter the port of the Database server[3306]: ")
    if DBPort == '':
        DBPort = '3306'
    while not(DBPort.strip().isdigit()):
        DBPort = input("Enter a valid port [3306]: ")
        if DBPort == '':
            DBPort = '3306'
    
    DBMaxConnections = input("Enter the maximum number of connections to the database: ")
    while DBMaxConnections == '' or not(DBMaxConnections.strip().isdigit()):
        DBMaxConnections = input("Enter a valid number: ")
    
    DBDatabase = input("Enter the name of the Database [EAR]: ")
    if DBDatabase == '':
        DBDatabase = 'EAR'
    while DBDatabase.strip().isdigit():
        DBDatabase = input("Enter a valid name [EAR]: ")
        if DBDatabase == '':
            DBDatabase = 'EAR'
    
    DBUser = input("Enter the username of the Database server [ear_daemon]: ")
    if DBUser == '':
        DBUser = 'ear_daemon'
    while DBUser.strip().isdigit():
        DBUser = input("Enter a valid username [ear_daemon]: ")
        if DBUser == '':
            DBUser = 'ear_daemon'
    
    DBPassw = input("Enter the password of the Database server [password]: ")
    
    DBCommands = input("Do you want a specific user for read-only access to the database [Y/n]: ")
    if DBCommands == '':
        DBCommands = 'y'
    while not(DBCommands in ['y', 'Y', 'n', 'N']) or DBCommands.strip().isdigit():
        DBCommands = input("[Y/n]: ")
        if DBCommands == '':
            DBCommands = 'y'
            
    if DBCommands in ['y', 'Y']:
        DBCommandsUser = input("Enter the username [ear_commands]: ")
        if DBCommandsUser == '':
            DBCommandsUser = 'ear_commands'
        DBCommandsPassw = input("Enter the password [password]: ")
        if DBCommandsPassw == '':
            DBCommandsPassw = 'password'
            
    if DBCommands in ['n', 'N']:
        DBCommandsUser = ''
        DBCommandsPassw = ''
    
    DBReportNodeDetail = input("Do you want to save the extended node information [Y/n]: ")
    if DBReportNodeDetail == '':
        DBReportNodeDetail = 'y'
    while not(DBReportNodeDetail in ['y', 'Y', 'n', 'N']) or DBReportNodeDetail.strip().isdigit():
        DBReportNodeDetail = input("[Y/n]: ")
        if DBReportNodeDetail == '':
            DBReportNodeDetail = 'y'
    if DBReportNodeDetail in ['y', 'Y']:
        DBReportNodeDetail = '1'
    if DBReportNodeDetail in ['n', 'N']:
        DBReportNodeDetail = '0'

    DBreportSIGDetail = input("Do you want to save the extended signature information [Y/n]: ")
    if DBreportSIGDetail == '':
        DBreportSIGDetail = 'y'
    while not(DBreportSIGDetail in ['y', 'Y', 'n', 'N']) or DBreportSIGDetail.strip().isdigit():
        DBreportSIGDetail = input("[Y/n]: ")
        if DBreportSIGDetail == '':
            DBreportSIGDetail = 'y'
    if DBreportSIGDetail in ['y', 'Y']:
        DBreportSIGDetail = '1'
    if DBreportSIGDetail in ['n', 'N']:
        DBreportSIGDetail = '0'
    
    DBReportLoops = input("Do you want to save application runtime metrics [Y/n]: ")
    if DBReportLoops == '':
        DBReportLoops = 'y'
    while not(DBReportLoops in ['y', 'Y', 'n', 'N']) or DBReportLoops.strip().isdigit():
        DBReportLoops = input("[Y/n]: ")
        if DBReportLoops == '':
            DBReportLoops = 'y'
    if DBReportLoops in ['y', 'Y']:
        DBReportLoops = '1'
    if DBReportLoops in ['n', 'N']:
        DBReportLoops = '0'

if DBPluginType in ['n', 'N']:
    print("The relational database plugin is configuered with default values")
    DBIp = '127.0.0.1'
    DBUser = 'ear_daemon'
    DBPassw = 'password'
    DBCommandsUser = 'ear_commands'
    DBCommandsPassw = 'password'
    DBDatabase = 'EAR'
    DBPort = '3306'
    DBMaxConnections = '20'
    DBReportNodeDetail = '1'
    DBreportSIGDetail = '1'
    DBReportLoops = '1'

print("============================= EAR Daemon (EARD) configuration =============================")
print("")

NodeDaemonPort = input("Enter the port number that will be used to connect with the EAR plugin and commands [50001]: ")
if NodeDaemonPort == '':
    NodeDaemonPort = '50001'
while not(NodeDaemonPort.strip().isdigit()):
    NodeDaemonPort = input("Enter a valid number [50001]: ")
    if NodeDaemonPort == '':
        NodeDaemonPort = '50001'

NodeDaemonPowermonFreq = input("Enter the frequency at which the periodic metrics are reported, in seconds [60]: ")
if NodeDaemonPowermonFreq == '':
    NodeDaemonPowermonFreq = '60'
while not(NodeDaemonPowermonFreq.strip().isdigit()):
    NodeDaemonPowermonFreq = input("Enter a valid number [60]: ")
    if NodeDaemonPowermonFreq == '':
        NodeDaemonPowermonFreq = '60'

NodeUseDB = input("Do you want the EARD to use the database [Y/n]: ")
if NodeUseDB == '':
    NodeUseDB = 'y'
while not(NodeUseDB in ['y', 'Y', 'n', 'N']) or NodeUseDB.strip().isdigit():
    NodeUseDB = input("[Y/n]: ")
    if NodeUseDB == '':
        NodeUseDB = 'y'
if NodeUseDB in ['y', 'Y']:
    NodeUseDB = '1'
if NodeUseDB in ['n', 'N']:
    NodeUseDB = '0'

NodeUseLog = input("Should the EARD use a log file? [Y/n]: ")
if NodeUseLog == '':
    NodeUseLog = 'y'
while not(NodeUseLog in ['y', 'Y', 'n', 'N']) or NodeUseLog.strip().isdigit():
    NodeUseLog = input("[Y/n]: ")
    if NodeUseLog == '':
        NodeUseLog = 'y'
if NodeUseLog in ['y', 'Y']:
        NodeUseLog = 1
if NodeUseLog in ['n', 'N']:
        NodeUseLog = 0

EARDReportPlugins = input("Enter the names of the plugins that you want to be loaded by the EARD (the first one is the default plugin to ue)\n[eardbd.so, csv.so, csv_ts.so, dcdb.so, dcgmi.so, eard.so, examon.so, mpi_node_metrics.so, mpitrace.so, mysql.so, prometheus.so, psql.so, sysfs.so]: ")
if EARDReportPlugins == '':
    EARDReportPlugins = 'eardbd.so'
EARDReportPlugins = EARDReportPlugins.replace(' ', '')
pluginsList = EARDReportPlugins.split(',')
flag = 0
for i in pluginsList:
    if not(str(i) in defaultPluginsList):
        flag += 1
while EARDReportPlugins.strip().isdigit() or flag > 0:
    EARDReportPlugins = input("Enter a valid plugin name: ")
    if EARDReportPlugins == '':
        EARDReportPlugins = 'eardbd.so'
    EARDReportPlugins = EARDReportPlugins.replace(' ', '')
    pluginsList = EARDReportPlugins.split(',')
    flag = 0
    for i in pluginsList:
        if not(str(i) in defaultPluginsList):
            flag += 1

print("============================= EAR Database Manager (EARDBD) configuration =============================")
print("")

EARDBDTypeList = []
EARDBDHostnameList = []

EARDBDType = input("Do you want the EARDBD to be a global per all types/partitions or configuered per type/partition\n[global/per-type]: ")
while EARDBDType.strip().isdigit():
    EARDBDType = input("Enter a valid value [global/per-type]: ")

if EARDBDType == 'global':
    globalNodeHostname = input("On which node the service should be hosted. Enter the hostname: ")

if EARDBDType == 'per-type':
    perTypeNodesType = input("Enter the partition type: ")
    EARDBDTypeList.append(perTypeNodesType)
    perTypeNodesHostname = input("Enter the hostname: ")
    EARDBDHostnameList.append(perTypeNodesHostname)
    perTypeNodes = 'y'
    while perTypeNodes in ['y', 'Y']:
        perTypeNodes = input("Do you want to add more types [y/N]: ")
        if perTypeNodes == '':
            perTypeNodes = 'n'
            break
        perTypeNodesType = input("Enter the partition type: ")
        EARDBDTypeList.append(perTypeNodesType)
        perTypeNodesHostname = input("Enter the hostname: ")
        EARDBDHostnameList.append(perTypeNodesHostname)

EARDBDMirror = input("DO you want to setup the mirror DB service on a different server [y/N]: ")
if EARDBDMirror == '':
    EARDBDMirror = 'n'
while not(EARDBDMirror in ['y', 'Y', 'n', 'N']):
    EARDBDMirror = input("[y/N]: ")
    if EARDBDMirror == '':
        EARDBDMirror = 'n'
if EARDBDMirror in ['y', 'Y']:
    EARDBDMirrorHostname = input("Enter the hostname: ")
    if EARDBDMirrorHostname == '':
        EARDBDMirrorHostname = input("Enter a valid hostname: ")

DBDaemonPortTCP = input("Enter the TCP port number that will be used by EARDBD Daemon [50002]: ")
if DBDaemonPortTCP == '':
    DBDaemonPortTCP = '50002'
while not(DBDaemonPortTCP.strip().isdigit()):
    DBDaemonPortTCP = input("Enter a valid number [50002]: ")
    if DBDaemonPortTCP == '':
        DBDaemonPortTCP = '50002'

DBDaemonPortSecTCP = input("Enter the SecTCP port number that will be used by EARDBD Daemon [50003]: ")
if DBDaemonPortSecTCP == '':
    DBDaemonPortSecTCP = '50003'
while not(DBDaemonPortSecTCP.strip().isdigit()):
    DBDaemonPortSecTCP = input("Enter a valid number [50003]: ")
    if DBDaemonPortSecTCP == '':
        DBDaemonPortSecTCP = '50003'

DBDaemonSyncPort = input("Enter the Sync port number that will be used by EARDBD Daemon [50004]: ")
if DBDaemonSyncPort == '':
    DBDaemonSyncPort = '50004'
while not(DBDaemonSyncPort.strip().isdigit()):
    DBDaemonSyncPort = input("Enter a valid number [50004]: ")
    if DBDaemonSyncPort == '':
        DBDaemonSyncPort = '50004'

DBDaemonAggregationTime = input("Enter the frequency at which the power metrics are aggregated in aggregated metrics, in seconds [60]: ")
if DBDaemonAggregationTime == '':
    DBDaemonAggregationTime = '60'
while not(DBDaemonAggregationTime.strip().isdigit()):
    DBDaemonAggregationTime = input("Enter a valid number [60]: ")
    if DBDaemonAggregationTime == '':
        DBDaemonAggregationTime = '60'

DBDaemonInsertionTime = input("Enter the frequency at which the buffered data is sent to DB server, in seconds [30]: ")
if DBDaemonInsertionTime == '':
    DBDaemonInsertionTime = '30'
while not(DBDaemonInsertionTime.strip().isdigit()):
    DBDaemonInsertionTime = input("Enter a valid number [30]: ")
    if DBDaemonInsertionTime == '':
        DBDaemonInsertionTime = '30'

DBDaemonUseLog = input("Should the EARDBD use a log file? [Y/n]: ")
if DBDaemonUseLog == '':
    DBDaemonUseLog = 'y'
while not(DBDaemonUseLog in ['y', 'Y', 'n', 'N']) or DBDaemonUseLog.strip().isdigit():
    DBDaemonUseLog = input("[Y/n]: ")
    if DBDaemonUseLog == '':
        DBDaemonUseLog = 'y'
if DBDaemonUseLog in ['y', 'Y']:
        DBDaemonUseLog = 1
if DBDaemonUseLog in ['n', 'N']:
        DBDaemonUseLog = 0

EARDBDReportPlugins = input("Enter the names of the plugins that you want to be loaded by the EARD (the first one is the default plugin to ue)\n[mysql.so, eardbd.so, csv.so, csv_ts.so, dcdb.so, dcgmi.so, eard.so, examon.so, mpi_node_metrics.so, mpitrace.so, prometheus.so, psql.so, sysfs.so]: ")
if EARDBDReportPlugins == '':
    EARDBDReportPlugins = 'mysql.so'
EARDBDReportPlugins = EARDBDReportPlugins.replace(' ', '')
pluginsList = EARDBDReportPlugins.split(',')
flag = 0
for i in pluginsList:
    if not(str(i) in defaultPluginsList):
        flag += 1
while EARDBDReportPlugins.strip().isdigit() or flag > 0:
    EARDBDReportPlugins = input("Enter a valid plugin name: ")
    if EARDBDReportPlugins == '':
        EARDBDReportPlugins = 'mysql.so'
    EARDBDReportPlugins = EARDBDReportPlugins.replace(' ', '')
    pluginsList = EARDBDReportPlugins.split(',')
    flag = 0
    for i in pluginsList:
        if not(str(i) in defaultPluginsList):
            flag += 1
'''
print("============================= EAR Global Manager (EARGMD) configuration =============================")
print("")
print("============================= Cluster Structure Data =============================")
print("")
print("To configure energy or power capping we need information about the cluster structure.")
print("")

clusterSize = input("How many types of nodes/partitions does the cluster contain (for example, CPU-only and CPU+2GPUs): ")
while clusterSize == '' or not(clusterSize.strip().isdigit()):
        clusterSize = input("How many types of nodes/partitions does the cluster contain: ")

clusterTypeName = input("Would you like to name the cluster type of nodes/partitions [Y/n]: ")
if clusterTypeName == '':
        clusterTypeName = 'y'
while not(clusterTypeName in ['y', 'Y', 'n', 'N']) or clusterTypeName.strip().isdigit():
        if clusterTypeName == '':
                clusterTypeName = 'y'
                break
        clusterTypeName = input("[Y/n]: ")

typesNameList = []
if clusterTypeName in ['y', 'Y']:
        names = input("Enter the names (use comma separated format)(i.e. type1,type2 or CPU-only,CPU-GPU): ")
        pattern1 = re.compile(r"^(\w+)(,\s*\w+)*$")
        pattern2 = re.compile(r"^(\w+-\w+)(,\s*\w+-\w+)*$")
        while names == '' or len(names.split(',')) != int(clusterSize):
                names = input("Enter a valid names format: ")
        typesNameList = (names.replace(" ", "")).split(",")

if clusterTypeName in ['n', 'N']:
        print("The default naming format will be used, like: type1, type2,....,typeN")
        for i in range(1, int(clusterSize)+1, 1):
                typesNameList.append("type" + str(i))
print("")

nodesNumList = []
maxPowerList = []
averagePowerList = []
for i in typesNameList:
        print("Configuring type of {}".format(i))
        nodesNum = input("How many nodes of this types are there: ")
        while nodesNum == '' or not(nodesNum.strip().isdigit()):
                nodesNum = input("Enter a valid number of nodes: ")
        nodesNumList.append(nodesNum)
        maxPower = input("What is the max power consumption (in Watts): ")
        while maxPower == '' or not(maxPower.strip().isdigit()):
                maxPower = input("Enter a valid max power consumption (in Watts): ")
        maxPowerList.append(maxPower)
        averagePower = input("What is the average power consumption (in Watts): ")
        while averagePower == '' or not(averagePower.strip().isdigit()):
                averagePower = input("Enter a valid average power consumption (in Watts): ")
        averagePowerList.append(averagePower)
        print("")
print("")

print("==========================")
print("= POWERCAP Configuration =")
print("==========================")
print("")

powerCap = input("Would you like to configure power capping [Y/n]: ")
if powerCap == '':
        powerCap = 'y'
while not(powerCap in ['y', 'Y', 'n', 'N']) or powerCap.strip().isdigit():
        if powerCap == '':
                powerCap = 'y'
                break
        powerCap = input("[Y/n]: ")

if powerCap in ['y', 'Y']:
        print("General powercap configuration:")

        EARGMPowerCapMode = input("What powercap mode should be used (monitor/0-hard powercap/1-soft powercap/2) [0/1/2]: ")
        while EARGMPowerCapMode == '' or not(EARGMPowerCapMode.strip().isdigit()) or not(EARGMPowerCapMode in ['0', '1', '2']):
                EARGMPowerCapMode = input("[0/1/2]: ")

        EARGMPowerPeriod = input("How often (in seconds) should the EARGMs ask the nodes for their power status [120]: ")
        if EARGMPowerPeriod == '':
                EARGMPowerPeriod = 120
        while not(str(EARGMPowerPeriod).strip().isdigit()):
                EARGMPowerPeriod = input("Enter a valid period (in seconds) [120]: ")
                if EARGMPowerPeriod == '':
                        EARGMPowerPeriod = 120

        powerLimit = input("Do you want to manage power limit for each partition independently [y/n]: ")
        while not(powerLimit in ['y', 'Y', 'n', 'N']) or powerLimit.strip().isdigit():
                powerLimit = input("[y/n]: ")

        eargmIdList = []
        eargmTypeList = []
        eargmPowerList = []
        eargmHostnameList = []
        eargmPortList = []

        if powerLimit in ['y', 'Y']:
                eargmS = 0
                nodesNum = 0
                for i in nodesNumList:
                        num = 0
                        if int(i) > 1000:
                                num = round(int(i)/1000)
                        eargmS = int(clusterSize) + num
                        nodesNum = nodesNum + int(i)

                eargmNum = input("How many EARGMs would you like to configure (Given {} nodes and {} partitions, we recommend {} EARGMs)? [{}]: ".format(str(nodesNum), str(clusterSize), str(eargmS), str(eargmS)))
                if eargmNum == '':
                        eargmNum = eargmS
                while not(str(eargmNum).strip().isdigit()):
                        eargmNum = input("Enter a valid EARGMs number, we recommend {} EARGMs)? [{}]: ".format(eargmS, eargmS))
                        if eargmNum == '':
                                eargmNum = eargmS
                print("")

                if int(eargmNum) == 1:
                        print("Configuring EARGM with ID 1")

                        eargmIdList.append(1)
                        eargmTypeList.append(0)
        
                        eargmPowerSug = 0
                        for (i, j) in zip(nodesNumList, maxPowerList):
                                val = int(i) * int(j)
                                eargmPowerSug = eargmPowerSug + val
                        
                        eargmPower = input("What is this EARGM's power budget [we suggest a budget of {}W]: ".format(int(eargmPowerSug)))
                        if eargmPower == '':
                                eargmPower = eargmPowerSug
                        while not(str(eargmPower).strip().isdigit()):
                                eargmPower = input("Enter a valid EARGM's power budget [we suggest a budget of {}W]: ".format(int(eargmPowerSug)))
                                if eargmPower == '':
                                        eargmPower = eargmPowerSug
                        eargmPowerList.append(int(eargmPower))
        
                        eargmHostname = input("What is the hostname where the EARGM will be running [nodename]: ")
                        if eargmHostname == '':
                                eargmHostname = 'nodename'
                        while eargmHostname.strip().isdigit():
                                eargmHostname = input("Enter a valid hostname [nodename]: ")
                                if eargmHostname == '':
                                        eargmHostname = 'nodename'
                        eargmHostnameList.append(eargmHostname)
        
                        eargmPort = input("What port will this EARGM use for remote connections [50100]: ")
                        if eargmPort == '':
                                eargmPort = '50100'
                        while not(eargmPort.strip().isdigit()):
                                eargmPort = input("Enter a valid port number [50100]: ")
                                if eargmPort == '':
                                        eargmPort = '50100'
                        eargmPortList.append(int(eargmPort))
                        print("")

                if int(eargmNum) > 1:
                        for i in range(1, int(eargmNum)+1):
                                print("Configuring EARGM with ID {}: ".format(i))

                                eargmIdList.append(int(i))

                                eargmType = input("What type of nodes will it control [{}]: ".format(','.join(map(str, typesNameList))))
                                while eargmType == '' or not(eargmType in typesNameList) or (eargmType in eargmTypeList):
                                        eargmType = input("Enter a valid type of nodes [{}]: ".format(','.join(map(str, typesNameList))))
                                eargmTypeList.append(eargmType)

                                index = typesNameList.index(eargmType)
                                eargmPowerSug = int(nodesNumList[index]) * int(maxPowerList[index])
                                eargmPower = input("What is this EARGM's power budget [we suggest a budget of {}W]: ".format(eargmPowerSug))
                                if eargmPower == '':
                                        eargmPower = eargmPowerSug
                                while not(str(eargmPower).strip().isdigit()):
                                        eargmPower = input("Enter a valid power budget [we suggest a budget of {}W]: ".format(eargmPowerSug))
                                        if eargmPower == '':
                                                eargmPower = eargmPowerSug
                                eargmPowerList.append(int(eargmPower))

                                eargmHostname = input("What is the hostname where the EARGM will be running [nodename]: ")
                                if eargmHostname == '':
                                        eargmHostname = 'nodename'
                                while eargmHostname.strip().isdigit():
                                        eargmHostname = input("Enter a valid hostname [nodename]: ")
                                        if eargmHostname == '':
                                                eargmHostname = 'nodename'
                                eargmHostnameList.append(eargmHostname)

                                eargmPort = input("What port will this EARGM use for remote connections [50100]: ")
                                if eargmPort == '':
                                        eargmPort = '50100'
                                while not(eargmPort.strip().isdigit()):
                                        eargmPort = input("Enter a valid port number [50100]: ")
                                        if eargmPort == '':
                                                eargmPort = '50100'
                                eargmPortList.append(int(eargmPort))
                                print("")

        if powerLimit in ['n', 'N']:
                eargmNum = 1
                print("Configuring EARGM with ID 1")

                eargmIdList.append(1)
                eargmTypeList.append(0)

                eargmPowerSug = 0
                for (i, j) in zip(nodesNumList, maxPowerList):
                        val = int(i) * int(j)
                        eargmPowerSug = eargmPowerSug + val
                
                eargmPower = input("What is this EARGM's power budget [we suggest a budget of {}W]: ".format(int(eargmPowerSug)))
                if eargmPower == '':
                        eargmPower = eargmPowerSug
                while not(str(eargmPower).strip().isdigit()):
                        eargmPower = input("Enter a valid EARGM's power budget [we suggest a budget of {}W]: ".format(int(eargmPowerSug)))
                        if eargmPower == '':
                                eargmPower = eargmPowerSug
                eargmPowerList.append(int(eargmPower))

                eargmHostname = input("What is the hostname where the EARGM will be running [nodename]: ")
                if eargmHostname == '':
                        eargmHostname = 'nodename'
                while eargmHostname.strip().isdigit():
                        eargmHostname = input("Enter a valid hostname [nodename]: ")
                        if eargmHostname == '':
                                eargmHostname = 'nodename'
                eargmHostnameList.append(eargmHostname)

                eargmPort = input("What port will this EARGM use for remote connections [50100]: ")
                if eargmPort == '':
                        eargmPort = '50100'
                while not(eargmPort.strip().isdigit()):
                        eargmPort = input("Enter a valid port number [50100]: ")
                        if eargmPort == '':
                                eargmPort = '50100'
                eargmPortList.append(int(eargmPort))
                print("")

        #print(eargmIdList)
        #print(eargmTypeList)
        #print(eargmPowerList)
        #print(eargmHostnameList)
        #print(eargmPortList)

        EARGMPowerCapSuspendConfig = input("Do you want to set up a command when the power consumption goes above a certain percentage of the total [Y/n]: ")
        if EARGMPowerCapSuspendConfig == '':
                EARGMPowerCapSuspendConfig = 'y'
        while not(EARGMPowerCapSuspendConfig in ['y', 'Y', 'n', 'N']) or EARGMPowerCapSuspendConfig.strip().isdigit():
                if EARGMPowerCapSuspendConfig == '':
                        EARGMPowerCapSuspendConfig = 'y'
                        break
                EARGMPowerCapSuspendConfig = input("[Y/n]: ")

        if EARGMPowerCapSuspendConfig in ['y', 'Y']:
                EARGMPowerCapSuspendLimit = input("What percentage would trigger the command [Current power budgets: [{}]] [85]: ".format(", ".join(str(i) for i in eargmPowerList)))
                if EARGMPowerCapSuspendLimit == '':
                        EARGMPowerCapSuspendLimit = '85'
                while not(EARGMPowerCapSuspendLimit.strip().isdigit()):
                        EARGMPowerCapSuspendLimit = input("Enter a valid power percentage [85]: ")
                        if EARGMPowerCapSuspendLimit == '':
                                EARGMPowerCapSuspendLimit = '85'

                EARGMPowerCapSuspendAction = input("Please introduce the absolute path to the command (It will be executed with the following arguments\ncommand_name current_power current_limit total_idle_nodes total_idle_power: ")
                while EARGMPowerCapSuspendAction == '' or EARGMPowerCapSuspendAction.strip().isdigit():
                        EARGMPowerCapSuspendAction = input("Enter a valid command: ")
                print("")

        if EARGMPowerCapSuspendConfig in ['n', 'N']:
                EARGMPowerCapSuspendLimit = 0
                EARGMPowerCapSuspendAction = "no_action"
                print("")

        EARGMPowerCapResumeConfig = input("Do you want to set up a command when the power consumption goes below a certain percentage of the total [Y/n]: ")
        if EARGMPowerCapResumeConfig == '':
                EARGMPowerCapResumeConfig = 'y'
        while not(EARGMPowerCapResumeConfig in ['y', 'Y', 'n', 'N']) or EARGMPowerCapResumeConfig.strip().isdigit():
                if EARGMPowerCapResumeConfig == '':
                        EARGMPowerCapResumeConfig = 'y'
                        break
                EARGMPowerCapResumeConfig = input("[Y/n]: ")

        if EARGMPowerCapResumeConfig in ['y', 'Y']:
                EARGMPowerCapResumeLimit = input("What percentage would trigger the command [Current power budgets: [{}]] [40]: ".format(", ".join(str(i) for i in eargmPowerList)))
                if EARGMPowerCapResumeLimit == '':
                        EARGMPowerCapResumeLimit = '40'
                while not(EARGMPowerCapResumeLimit.strip().isdigit()):
                        EARGMPowerCapResumeLimit = input("Enter a valid power percentage [40]: ")
                        if EARGMPowerCapResumeLimit == '':
                                EARGMPowerCapResumeLimit = '40'

                EARGMPowerCapResumeAction = input("\nPlease introduce the absolute path to the command (It will be executed with the following arguments\ncommand_name current_power current_limit total_idle_nodes total_idle_power): ")
                while EARGMPowerCapResumeAction == '' or EARGMPowerCapResumeAction.strip().isdigit():
                        EARGMPowerCapResumeAction = input("Enter a valid command: ")
                print("")

        if EARGMPowerCapResumeConfig in ['n', 'N']:
                EARGMPowerCapResumeLimit = 0
                EARGMPowerCapResumeAction = "no_action"
                print("")


        if int(eargmNum) > 1:
                metaEargm = input("Would you like to allow power budget redistribution between EARGMs? [Y/n]: ")
                if metaEargm == '':
                        metaEargm = 'Y'
                while not(metaEargm in ['y', 'Y', 'n', 'N']) or metaEargm.strip().isdigit():
                        metaEargm = input("[Y/n]: ")
                        if metaEargm == '':
                                metaEargm = 'Y'

                if metaEargm in ['y', 'Y']:
                        metaEargmNum = input("How many meta-EARGMs do you want to configure: ")
                        while metaEargmNum == '' or not(metaEargmNum.strip().isdigit()):
                                metaEargmNum = input("Enter a valid number: ")

                        EargmIds = []
                        for i in range(1, int(eargmNum)+1):
                                EargmIds.append(i)

                        EargmIds2 = []
                        for i in range(1, int(eargmNum)+1):
                                EargmIds2.append(i)

                        metaEargmIdlst = []
                        metaEargmIdslst = []
                        for i in range(1, int(metaEargmNum)+1):
                                print("Configuring meta-EARGM with ID {}: ".format(i))

                        metaEargmId = input("Which EARGM would be in charge of managing the distribution? [{}]: ".format("/".join(str(i) for i in EargmIds)))
                        while metaEargmId == '' or not(metaEargmId.strip().isdigit()) or int(metaEargmId) not in EargmIds:
                                metaEargmId = input("Enter a valid EARGM ID: ")

                        EargmIds.remove(int(metaEargmId))
                        metaEargmIdlst.append(metaEargmId)

                        metaEargmIds = input("What EARGMs should it control (comma separated list)? [{}]: ".format(", ".join(str(i) for i in EargmIds2)))
                        pattern = re.compile(r"^(\w+)(,\s*\w+)*$")
                        metaEargmIdsList = metaEargmIds.split(",")
                        while metaEargmIds == '' or pattern.match(metaEargmIds) == None or not all(int(x) in EargmIds2 for x in metaEargmIdsList):
                                metaEargmIds = input("Enter a valid EARGM IDs: ")
                                metaEargmIdsList = metaEargmIds.split(",")

                        for i in metaEargmIdsList:
                                EargmIds2.remove(int(i))

                        metaEargmIdslst.append(metaEargmIdsList)
                        print("")

                if metaEargm in ['n', 'N']:
                        print("No meta-EARGM is configured")
                        metaEargmIdlst = [0] * len(eargmIdList)
                        metaEargmIdslst = [0] * len(eargmIdList)
                print("")

if powerCap in ['n', 'N']:
        print("No Power Capping is configured\n")
        EARGMPowerCapMode = 0
        EARGMPowerPeriod = 0
        EARGMPowerCapSuspendLimit = 0
        EARGMPowerCapSuspendAction = "no_action"
        EARGMPowerCapResumeLimit = 0
        EARGMPowerCapResumeAction = "no_action"
print("")

print("================================")
print("= ENERGY CAPPING Configuration =")
print("================================")
print("")

energyCap = input("Would you like to configure energy capping? [Y/n]: ")
if energyCap == '':
        energyCap = 'y'
while not(energyCap in ['y', 'Y', 'n', 'N']) or energyCap.strip().isdigit():
        if energyCap == '':
                energyCap = 'y'
                break
        energyCap = input("[Y/n]: ")

if energyCap in ['y', 'Y']:
        
        T1 = input("How often should the EARGM monitor for energy status (in seconds)? [600]: ")
        if T1 == '':
                T1 = 600
        while not(str(T1).strip().isdigit()):
                T1 = input("Enter a valid time (in seconds)? [600]: ")
                if T1 == '':
                        T1 = 600

        T2 = input("What time period (in seconds) should the EARGM use to monitor the energy (for example, the last month)? [2592000]: ")
        if T2 == '':
                T2 = '2592000'
        while not(str(T2).strip().isdigit()):
                T2 = input("Enter a valid time (in seconds)? [2592000]: ")
                if T2 == '':
                        T2 = '2592000'

        EARGMUnits = input("In which unit do you want to specify the energy? [J/K/M]: ")
        if EARGMUnits == 'j':
                EARGMUnits ='J'
        if EARGMUnits == 'k':
                EARGMUnits ='K'
        if EARGMUnits == 'm':
                EARGMUnits ='M'
        while EARGMUnits == '' or EARGMUnits.strip().isdigit() or not(EARGMUnits in ['j', 'J', 'k', 'K', 'm', 'M']):
                EARGMUnits = input("Enter a valid unit [J/K/M]: ")
                if EARGMUnits == 'j':
                        EARGMUnits ='J'
                if EARGMUnits == 'k':
                        EARGMUnits ='K'
                if EARGMUnits == 'm':
                        EARGMUnits ='M'

        energy_total = 0
        for (i, j) in zip(nodesNumList, averagePowerList):
                energy = int(i) * int(j)
                energy_total += energy

        if EARGMUnits in ['j', 'J']:
                unit = 1
        if EARGMUnits in ['k', 'K']:
                unit = 1000
        if EARGMUnits in ['m', 'M']:
                unit = 1000000

        limit = (int(T2) * energy_total)/int(unit)

        energyLimit = input("What is the target energy limit? [{}]: ".format(int(limit)))
        if energyLimit == '':
                energyLimit = int(limit)
        while not(str(energyLimit).strip().isdigit()):
                energyLimit = input("Enter a valid number [{}]: ".format(int(limit)))
                if energyLimit == '':
                        energyLimit = int(limit)

        EARGMUseAggregated = input("To do energy control, an EARGM will read data from the configured database.\nShould it use the aggregated data (better performance) instead of the per-node data? [Y/n]: ")
        if EARGMUseAggregated == '':
                EARGMUseAggregated = 'y'
        while not(EARGMUseAggregated in ['y', 'Y', 'n', 'N']) or EARGMUseAggregated.strip().isdigit():
                EARGMUseAggregated = input("[Y/n]: ")
                if EARGMUseAggregated == '':
                        EARGMUseAggregated = 'y'
        if EARGMUseAggregated in ['y', 'Y']:
                EARGMUseAggregated = 1
        if EARGMUseAggregated in ['n', 'N']:
                EARGMUseAggregated = 0
       
        EARGMWarnings = input("Would you like to set up warnings when certain energy thresholds are passed? [Y/n]: ")
        if EARGMWarnings == '':
                EARGMWarnings = 'y'
        while not(EARGMWarnings in ['y', 'Y', 'n', 'N']) or EARGMWarnings.strip().isdigit():
                EARGMWarnings = input("[Y/n]: ")
                if EARGMWarnings == '':
                        EARGMWarnings = 'y'

        if EARGMWarnings in ['y', 'Y']:
                EARGMWarningsPerc = input("Please introduce the thresholds separated by a comma [85,90,95]: ")
                if EARGMWarningsPerc == '':
                        EARGMWarningsPerc = '85,90,95'
                pattern = re.compile(r"^(\w+)(,\s*\w+)*$")
                flag = 0
                i = 1
                numbers = [int(x.strip()) for x in EARGMWarningsPerc.split(',')]
                while i < len(numbers):
                        if(numbers[i] < numbers[i - 1]):
                                flag = 1
                        i += 1
                while pattern.match(EARGMWarningsPerc) == None or (len([int(x.strip()) for x in EARGMWarningsPerc.split(',')]) != 3) or flag > 0:
                        EARGMWarningsPerc = input("Enter a valid three values from lowest to highest with comma separated format [85,90,95]: ")
                        if EARGMWarningsPerc == '':
                                EARGMWarningsPerc = '85,90,95'
                        pattern = re.compile(r"^(\w+)(,\s*\w+)*$")
                        flag = 0
                        i = 1
                        numbers = [int(x.strip()) for x in EARGMWarningsPerc.split(',')]
                        while i < len(numbers):
                                if(numbers[i] < numbers[i - 1]):
                                        flag = 1
                                i += 1

                Email = input("Do you want to receive an email when the thresholds are passed? [Y/n]: ")
                if Email == '':
                        Email = 'y'
                while not(Email in ['y', 'Y', 'n', 'N']) or Email.strip().isdigit():
                        Email = input("[Y/n]: ")
                        if Email == '':
                                Email = 'y'

                if Email in ['y', 'Y']:
                        EARGMMail = input("Please introduce an email address: ")
                        while EARGMMail == '' or EARGMMail.strip().isdigit():
                                EARGMMail = input("Enter a valid email address: ")

                if Email in ['n', 'N']:
                        EARGMMail = 'nomail'

                EARGMEnergyAction = input("Do you want to execute a command each time a threshold is reached? [Y/n]: ")
                if EARGMEnergyAction == '':
                        EARGMEnergyAction = 'y'
                while not(EARGMEnergyAction in ['y', 'Y', 'n', 'N']) or Email.strip().isdigit():
                        EARGMEnergyAction = input("[Y/n]:")
                        if EARGMEnergyAction == '':
                                EARGMEnergyAction = 'y'

                if EARGMEnergyAction in ['y', 'Y']:
                        EARGMEnergyActionCommand = input("Please introduce the absolute path to the command (It will be executed with the following arguments\ncommand_name energy_T1 energy_T2  energy_limit T2 T1  units: ")
                        while EARGMEnergyActionCommand == '' or EARGMEnergyActionCommand.strip().isdigit():
                                EARGMEnergyActionCommand = input("Enter a valid command: ")
                        print("")

                if EARGMEnergyAction in ['n', 'N']:
                        EARGMEnergyActionCommand = "no_action"
                print("")

        if EARGMWarnings in ['n', 'N']:
                print("No energy thresholds configured")
                EARGMWarningsPerc = '0,0,0'
                EARGMMail = 'nomail'
                EARGMEnergyActionCommand = "no_action"
                print("")

if energyCap in ['n', 'N']:
        print("No energy capping is configured")
        EARGMUseAggregated = 1
        T1 = 0
        T2 = 0
        EARGMUnits = 'J'
        EARGMMail = 'nomail'
        EARGMWarningsPerc = '0,0,0'
        EARGMEnergyActionCommand = "no_action"
        energyLimit = 0
        print("")

print("")
EARGMUseLog = input("Should the EARGM use a log file? [Y/n]: ")
if EARGMUseLog == '':
        EARGMUseLog = 'y'
while not(EARGMUseLog in ['y', 'Y', 'n', 'N']) or EARGMUseLog.strip().isdigit():
        EARGMUseLog = input("[Y/n]: ")
        if EARGMUseLog == '':
                EARGMUseLog = 'y'
if EARGMUseLog in ['y', 'Y']:
        EARGMUseLog = 1
if EARGMUseLog in ['n', 'N']:
        EARGMUseLog = 0
print("")

print("============================= Authorized Users =============================")
print("")

AuthorizedUsers = input("Enter the list of the authorized users (use comma separated format): ")
while AuthorizedUsers.strip().isdigit():
    AuthorizedUsers = input("Enter a valid format (use comma separated format): ")

AuthorizedAccounts = input("Enter the list of the authorized accounts (use comma separated format): ")
while AuthorizedAccounts.strip().isdigit():
    AuthorizedAccounts = input("Enter a valid format (use comma separated format): ")

AuthorizedGroups = input("Enter the list of the authorized groups (use comma separated format): ")
while AuthorizedGroups.strip().isdigit():
    AuthorizedGroups = input("Enter a valid format (use comma separated format): ")
print("")

print("============================= Tags =============================")
print("")

max_avx512List = []
max_avx2List = []
min_powerList = []
gpuFreqList = []
energyPluginList = []
energyModelList= []
idle_governorList = []
defaultTagList = []

for i in typesNameList:
    print("the partition type name: ", str(i))

    avx512 = input("Dose this type use avx512 instructions [Y/n]: ")
    if avx512 == '':
        avx512 = 'y'
    while not(avx512 in ['y', 'Y', 'n', 'N']) or avx512.strip().isdigit():
        avx512 = input("[Y/n]: ")
        if avx512 == '':
            avx512 = 'y'
    if avx512 in ['y', 'Y']:
        max_avx512 = input("Enter the max speed/frequency to use: ")
        while max_avx512 == '' or not(max_avx512.strip().isdigit()):
            max_avx512 = input("Enter a valid number: ")
    if avx512 in ['n', 'N']:
        max_avx512 = '0'
    max_avx512List.append(max_avx512)

    avx2 = input("Dose this type use avx2 instructions [Y/n]: ")
    if avx2 == '':
        avx2 = 'y'
    while not(avx2 in ['y', 'Y', 'n', 'N']) or avx2.strip().isdigit():
        avx2 = input("[Y/n]: ")
        if avx2 == '':
            avx2 = 'y'
    if avx2 in ['y', 'Y']:
        max_avx2 = input("Enter the max speed/frequency to use: ")
        while max_avx2 == '' or not(max_avx2.strip().isdigit()):
            max_avx2 = input("Enter a valid number: ")
    if avx2 in ['n', 'N']:
        max_avx2 = '0'
    max_avx2List.append(max_avx2)

    min_power = input("Enter the min_power amount to use when the node is idle: ")
    while not(min_power.strip().isdigit()):
        min_power = input("Enter a valid number: ")
    min_powerList.append(min_power)

    gpu = input("Dose the nodes of this type use GPUs [y/N]: ")
    if gpu == '':
        gpu = 'n'
    while not(gpu in ['y', 'Y', 'n', 'N']):
        gpu = input("[y/N]: ")
        if gpu == '':
            gpu = 'n'
    if gpu in ['y', 'Y']:
        gpuFreq = input("what is the default GPU frequency to use: ")
        while gpuFreq == '' or not(gpuFreq.strip().isdigit()):
            gpuFreq = input("Enter a valid number: ")
    if gpu in ['n', 'N']:
        gpuFreq = '0'
    gpuFreqList.append(gpuFreq)

    pluginList = ['nm', 'dcmi', 'inm_power', 'inm_power_freeipmi', 'rapl', 'redfish', 'sd650', 'SD650N_V2']
    energyPlugin = input("Which energy plugin do you want to use {}: ".format(pluginList))
    if energyPlugin == '':
        energyPlugin = 'nm'
    while not(energyPlugin in pluginList):
        energyPlugin = input("Enter a vlid name form the list {}: ".format(pluginList))
        if energyPlugin == '':
            energyPlugin = 'nm'
    energyPluginM = 'energy_'+energyPlugin+'.so '
    energyPluginList.append(energyPluginM)
    
    energyModel = input("Which interface do you want to use to read the energy: ")
    if energyModel == '':
        energyModel = 'default'
    while energyModel.strip().isdigit():
        energyModel = input("Enter a valid model name: ")
        if energyModel == '':
            energyModel = 'default'
    energyModelList.append(energyModel)
    
    idle_governor = input("Which governor do you want to use when the node is idle [default]: ")
    if idle_governor == '':
        idle_governor = 'default'
    while idle_governor.strip().isdigit():
        idle_governor = input("Enter a valid governor name [default]: ")
        if idle_governor == '':
            idle_governor = 'default'
    idle_governorList.append(idle_governor)
    
    if not(1 in defaultTagList):
        defaultTag = input("Do you want to use this type tag as default tag [y/n]: ")
        while not(defaultTag in ['y', 'Y', 'n', 'N']):
            defaultTag = input("[y/n]: ")
        if defaultTag in ['y', 'Y']:
            defaultTagList.append(1)
        if defaultTag in ['n', 'N']:
            defaultTagList.append(0)
    else:
        defaultTagList.append(0)
print("")

print("====================================================================================================")
print("earConf output file has been created")
print("====================================================================================================")
print("")
from contextlib import redirect_stdout
with open('earConf.txt', 'w') as f:
    with redirect_stdout(f):   
        print("# EAR Configuration File")
        print("")
        '''
        print("#---------------------------------------------------------------------------------------------------")
        print("# DB confguration: This configuration conrrespondons with the DB server installation.")
        print("#---------------------------------------------------------------------------------------------------")
        print("DBIp=", DBIp, sep='')
        print("DBUser=", DBUser, sep='')
        print("DBPassw=", DBPassw, sep='')
        print("# User and password for usermode querys.")
        print("DBCommandsUser=", DBCommandsUser, sep='')
        print("DBCommandsPassw=", DBCommandsPassw, sep='')
        print("DBDatabase=", DBDatabase, sep='')
        print("DBPort=", DBPort, sep='')
        print("DBMaxConnections=", DBMaxConnections, sep='')
        print("# Extended node information saves also the average frequency and temperature.")
        print("DBReportNodeDetail=", DBReportNodeDetail, sep='')
        print("# Extended signature information saves also the hardware counters.")
        print("DBreportSIGDetail=", DBreportSIGDetail, sep='')
        print("# Report loop signatures.")
        print("DBReportLoops=", DBReportLoops, sep='')
        print("#---------------------------------------------------------------------------------------------------")
        print("# EAR Daemon (EARD): Update this section to change EARD configuration.")
        print("#---------------------------------------------------------------------------------------------------")
        print("# Port is used for connections with the EAR plugin and commands.")
        print("NodeDaemonPort=", NodeDaemonPort, sep='')
        print("# Frequency at wich the periodic metrics are reported, in seconds.")
        print("NodeDaemonPowermonFreq=", NodeDaemonPowermonFreq, sep='')
        print("# Max frequency used by eard. It's max frequency but min pstate.")
        print("NodeDaemonMinPstate=0")
        print("NodeDaemonTurbo=0")
        print("# Defines whether EARD uses the DB.")
        print("NodeUseDB=", NodeUseDB, sep='')
        print("# Defines if EARD connects with EARDBD to report data or directly with the DB server. Only for testing.")
        print("NodeUseEARDBD=1")
        print("# When set to 1, this flag means EARD must set frequencies before job starts. If not, frequency is only changed in case job runs with EARL.")
        print("NodeDaemonForceFrequencies=1")
        print("# Verbosity.")
        print("NodeDaemonVerbose=1")
        print("# When set to 1, the output is saved in 'TmpDir'/eard.log (common configuration) as a log file.")
        print("NodeUseLog=", NodeUseLog, sep='')
        print("EARDReportPlugins=", EARDReportPlugins.replace(',', ':'), sep='')
        print("#---------------------------------------------------------------------------------------------------")
        print("# EAR Database Manager (EARDBD): Update this section to change EARDBD configuration.")
        print("#---------------------------------------------------------------------------------------------------")
        print("DBDaemonPortTCP=", DBDaemonPortTCP, sep='')
        print("DBDaemonPortSecTCP=", DBDaemonPortSecTCP, sep='')
        print("DBDaemonSyncPort=", DBDaemonSyncPort, sep='')
        print("# Aggregation time id frequency at which power metrics are aggregated in aggregated metrics, in seconds.")
        print("DBDaemonAggregationTime=", DBDaemonAggregationTime, sep='')
        print("# Frequency at which buffered data is sent to DB server.")
        print("DBDaemonInsertionTime=", DBDaemonInsertionTime, sep='')
        print("# Memory size expressed in MB per process (server and/or mirror) to cache the values.")
        print("DBDaemonMemorySize=120")
        print("#")
        print("# The percentage of the memory buffer used by the previous field, by each type.")
        print("# These types are: mpi, non-mpi and learning applications, loops, energy metrics and aggregations and events, in that order. If a type gets 0% of space, this metric is discarded and not saved into the database.")
        print("#")
        print("#DBDaemonMemorySizePerType=40,20,5,24,5,1,5")
        print("")
        print("# When set to 1, the output is saved in 'TmpDir'/eardbd.log (common configuration) as a log file.")
        print("DBDaemonUseLog=", DBDaemonUseLog, sep='')
        print("EARDBDReportPlugins=", EARDBDReportPlugins, sep='')
        print("#---------------------------------------------------------------------------------------------------")
        print("# EAR Library (EARL): These options modify internal EARL behaviour. Do not modify except you are an expert.")
        print("#---------------------------------------------------------------------------------------------------")
        print("CoefficientsDir=@sysconfdir@/ear/coeffs")
        print("# Default power policy values")
        print("MinTimePerformanceAccuracy=10000000")
        print("# DynAIS configuration")
        print("DynAISLevels=10")
        print("DynAISWindowSize=200")
        print("#")
        print("# Maximum time (in seconds) EAR will wait until a signature is computed.")
        print("# After 'DynaisTimeout' seconds, if no signature is computed, EAR will go to periodic mode.")
        print("#")
        print("DynaisTimeout=15")
        print("# When EAR goes to periodic mode, it will compute the application signature every 'LibraryPeriod' seconds.")
        print("LibraryPeriod=10")
        print("# EAR will check every N mpi calls whether it must go to periodic mode or not.")
        print("CheckEARModeEvery=1000")
        print("# EAR library default report plugin")
        print("EARLReportPlugins=eard.so")
        '''
        print("#---------------------------------------------------------------------------------------------------")
        print("# EAR Global Manager (EARGMD): Update that section to use EARGM.")
        print("#---------------------------------------------------------------------------------------------------")
        print("#")
        print("# Use aggregated periodic metrics or periodic power metrics.")
        print("# Aggregated metrics are only available when EARDBD is running.")
        print("#")
        print("EARGMUseAggregated=", EARGMUseAggregated, sep='')
        print("# Period T1 and T2 are specified in seconds (ex. T1 must be less than T2, ex. 10min and 1 month).")
        print("EARGMPeriodT1=", T1, sep='')
        print("EARGMPeriodT2=", T2, sep='')
        print("# '-' are Joules, 'K' KiloJoules and 'M' MegaJoules.")
        print("EARGMUnits=", EARGMUnits, sep='')
        print("EARGMPort=50000")
        print("# Two modes are supported '0=manual' and '1=automatic'.")
        print("EARGMMode=1")
        print("# Email address to report the warning level (and the action taken in automatic mode).")
        print("EARGMMail=", EARGMMail, sep='')
        print("# Percentage of accumulated energy to start the warning DEFCON level L4, L3 and L2.")
        print("EARGMWarningsPerc=", EARGMWarningsPerc.replace(" ",""), sep='')
        print("# T1 \"grace\" periods between DEFCON before re-evaluate.")
        print("EARGMGracePeriods=3")
        print("# Verbosity")
        print("EARGMVerbose=0")
        print("# When set to 1, the output is saved in 'TmpDir'/eargmd.log (common configuration) as a log file.")
        print("EARGMUseLog=", EARGMUseLog, sep='')
        print("#")
        print("# Format for action is: command_name energy_T1 energy_T2  energy_limit T2 T1  units")
        print("# This action is automatically executed at each warning level (only once per grace periods).")
        print("#")
        print("EARGMEnergyAction=", EARGMEnergyActionCommand, sep='')
        print("# Period at which the powercap thread is activated. Meta-EARGM checks the EARGMs it controls every 2*EARGMPowerPeriod")
        print("EARGMPowerPeriod=", EARGMPowerPeriod, sep='')
        print("# Powercap mode: 0 is monitoring, 1 is hard powercap, 2 is soft powercap.")
        print("EARGMPowerCapMode=", EARGMPowerCapMode, sep='')
        print("# Admins can specify to automatically execute a command in EARGMPowerCapSuspendAction when total_power >= EARGMPowerLimit*EARGMPowerCapResumeLimit/100")
        print("EARGMPowerCapSuspendLimit=", EARGMPowerCapSuspendLimit, sep='')
        print("# Format for action is: command_name current_power current_limit total_idle_nodes total_idle_power")
        print("EARGMPowerCapSuspendAction=", EARGMPowerCapSuspendAction, sep='')
        print("#")
        print("# Admins can specify to automatically execute a command in EARGMPowerCapResumeAction to undo EARGMPowerCapSuspendAction")
        print("# when total_power >= EARGMPowerLimit*EARGMPowerCapResumeLimit/100.")
        print("# Note that this will only be executed if a suspend action was executed previously.")
        print("#")
        print("EARGMPowerCapResumeLimit=", EARGMPowerCapResumeLimit, sep='')
        print("# Format for action is: command_name current_power current_limit total_idle_nodes total_idle_power")
        print("EARGMPowerCapResumeAction=", EARGMPowerCapResumeAction, sep='')
        print("#")
        print("# EARGMs must be specified with a unique id, their node and the port that receives remote")
        print("# connections. An EARGM can also act as meta-eargm if the meta field is filled, and it will")
        print("# control the EARGMs whose ids are in said field. If two EARGMs are in the same node,")
        print("# setting the EARGMID environment variable overrides the node field and chooses the characteristics")
        print("# of the EARGM with the correspoding id.")
        print("#")
        print("# Only one EARGM can currently control the energy caps, so setting the rest to 0 is recommended.")
        print("# energy = 0 -> energy_cap disabled")
        print("# power = 0  -> powercap disabled")
        print("# power = N  -> powercap budget for that EARGM (and the nodes it controls) is N")
        print("# power = -1 -> powercap budget is calculated by adding up the powercap set to each of the nodes under its control.")
        print("#")
        print("#")
        if powerCap in ['n', 'N']:
                print("#EARGMId=0 energy=0 power=0")
        
        if powerCap in ['y', 'Y'] and powerLimit in ['n', 'N']:
                print("EARGMId=1 energy={} power={} node={} port={}".format(energyLimit, eargmPowerList[0], eargmHostnameList[0], eargmPortList[0]))
        
        if powerCap in ['y', 'Y'] and powerLimit in ['y', 'Y']:
                for i in range(len(eargmIdList)):
                        if i == 0:
                                if str(eargmIdList[i]) in metaEargmIdlst:
                                        print("EARGMId={} energy={} power={} node={} port={} meta={}".format(eargmIdList[i], energyLimit, eargmPowerList[i], eargmHostnameList[i], eargmPortList[i], ','.join(metaEargmIdslst[metaEargmIdlst.index(str(eargmIdList[i]))])))
                                else:
                                        print("EARGMId={} energy={} power={} node={} port={}".format(eargmIdList[i], energyLimit, eargmPowerList[i], eargmHostnameList[i], eargmPortList[i]))
        
                        else:
                                if str(eargmIdList[i]) in metaEargmIdlst:
                                        print("EARGMId={} energy=0 power={} node={} port={} meta={}".format(eargmIdList[i], eargmPowerList[i], eargmHostnameList[i], eargmPortList[i], ','.join(metaEargmIdslst[metaEargmIdlst.index(str(eargmIdList[i]))])))
                                else:
                                        print("EARGMId={} energy=0 power={} node={} port={}".format(eargmIdList[i], eargmPowerList[i], eargmHostnameList[i], eargmPortList[i]))                        
        print("#")
        print("#---------------------------------------------------------------------------------------------------")
        print("# Common configuration")
        print("#---------------------------------------------------------------------------------------------------")
        print("TmpDir=@localstatedir@")
        print("EtcDir=@sysconfdir@")
        print("InstDir=@prefix@")
        print("Verbose=0")
        print("# Network extension (using another network instead of the local one).")
        print("# If compute nodes must be accessed from login nodes with a network different than default,")
        print("# and can be accesed using a expension, uncommmet next line and define 'netext' accordingly.")
        print("#NetworkExtension=netext")
        print("#---------------------------------------------------------------------------------------------------")
        print("# Authorized Users")
        print("#---------------------------------------------------------------------------------------------------")
        print("#")
        print("# Authorized users,accounts and groups are allowed to change policies, thresholds, frequencies, etc.")
        print("# They are supposed to be admins, all special name is supported.")
        print("#")
        print("AuthorizedUsers=", AuthorizedUsers.replace(" ",""), sep='')
        print("AuthorizedAccounts=", AuthorizedAccounts.replace(" ",""), sep='')
        print("AuthorizedGroups=", AuthorizedGroups.replace(" ",""), sep='')
        print("#---------------------------------------------------------------------------------------------------")
        print("# Tags")
        print("#---------------------------------------------------------------------------------------------------")
        print("# Tags are used for architectural descriptions. Max. AVX frequencies are used in predictor models")
        print("# and are SKU-specific. Max. and min. power are used for warning and error tracking.") 
        print("# Powercap specifies the maximum power a node is allowed to use by default. If an EARGM is")
        print("# controlling the cluster with mode UNLIMITED (powercap=1) max_powercap is the set power that")
        print("# a node will receive if the cluster needs to be power capped (otherwise it runs")
        print("# with unlimited power). A different than the default powercap plugin can be specified for nodes ")
        print("# using the tag. POWERCAP=0 --> disabled, POWERCAP=1 -->unlimited, POWERCAP=N (> 1) limits node to N watts")
        print("# At least a default tag is mandatory to be included in this file for a cluster to work properly.")
        print("#")
        print("#")
        print("# List of accepted options is: max_avx512(GHz), max_avx2(GHz), max_power(W), min_power(W), error_power(W), coeffs(filename), ")
        print("# powercap(W), powercap_plugin(filename), energy_plugin(filename), gpu_powercap_plugin(filename), max_powercap(W), gpu_def_freq(GHz), ")
        print("# cpu_max_pstate(0..max_pstate), imc_max_pstate(0..max_imc_pstate), energy_model(filename)")
        print("# imc_max_freq, imc_min_freq, idle_governor(def default), idle_pstate")
        print("#")
        print("#")
        #if powerCap in ['n', 'N'] or powerLimit in ['n', 'N']:
        if powerCap in ['n', 'N']:
                for i in range(len(typesNameList)):
                    if defaultTagList[i] == 1 and int(gpuFreqList[i]) == 0:
                        print("Tag={} default=yes max_avx512={} max_avx2={} max_power={} min_power={} error_power={} coeffs=coeffs.default powercap=0 powercap_plugin=dvfs.so energy_plugin={} max_powercap=0 cpu_max_pstate=0 imc_max_pstate=0 energy_model={} imc_max_freq=0 imc_min_freq=0 idle_governor={} idle_pstate=0".format(typesNameList[i], max_avx512List[i], max_avx2List[i], maxPowerList[i], min_powerList[i], int(maxPowerList[i])+(0.1*int(maxPowerList[i])), energyPluginList[i], energyModelList[i], idle_governorList[i]))
                    if defaultTagList[i] == 1 and int(gpuFreqList[i]) > 0:
                        print("Tag={} default=yes max_avx512={} max_avx2={} max_power={} min_power={} error_power={} coeffs=coeffs.default powercap=0 powercap_plugin=dvfs.so energy_plugin={} gpu_powercap_plugin=gpu.so max_powercap=0 gpu_def_freq={} cpu_max_pstate=0 imc_max_pstate=0 energy_model={} imc_max_freq=0 imc_min_freq=0 idle_governor={} idle_pstate=0".format(typesNameList[i], max_avx512List[i], max_avx2List[i], maxPowerList[i], min_powerList[i], int(maxPowerList[i])+(0.1*int(maxPowerList[i])), energyPluginList[i], gpuFreqList[i], energyModelList[i], idle_governorList[i]))
                    if defaultTagList[i] == 0 and int(gpuFreqList[i]) == 0:
                        print("Tag={} max_avx512={} max_avx2={} max_power={} min_power={} error_power={} coeffs=coeffs.default powercap=0 powercap_plugin=dvfs.so energy_plugin={} max_powercap=0 cpu_max_pstate=0 imc_max_pstate=0 energy_model={} imc_max_freq=0 imc_min_freq=0 idle_governor={} idle_pstate=0".format(typesNameList[i], max_avx512List[i], max_avx2List[i], maxPowerList[i], min_powerList[i], int(maxPowerList[i])+(0.1*int(maxPowerList[i])), energyPluginList[i], energyModelList[i], idle_governorList[i]))
                    if defaultTagList[i] == 0 and int(gpuFreqList[i]) > 0:
                        print("Tag={} max_avx512={} max_avx2={} max_power={} min_power={} error_power={} coeffs=coeffs.default powercap=0 powercap_plugin=dvfs.so energy_plugin={} gpu_powercap_plugin=gpu.so max_powercap=0 gpu_def_freq={} cpu_max_pstate=0 imc_max_pstate=0 energy_model={} imc_max_freq=0 imc_min_freq=0 idle_governor={} idle_pstate=0".format(typesNameList[i], max_avx512List[i], max_avx2List[i], maxPowerList[i], min_powerList[i], int(maxPowerList[i])+(0.1*int(maxPowerList[i])), energyPluginList[i], gpuFreqList[i], energyModelList[i], idle_governorList[i]))

        if powerCap in ['y', 'Y'] and EARGMPowerCapMode in ['0', '2'] and powerLimit in ['y', 'Y']:
                for i in range(len(typesNameList)):
                    percentage = ((abs(int(eargmPowerList[i])) - eargmPowerSug)*100)/eargmPowerSug
                    #if int(i) == 0:
                        #i == 'all_types'
                    if defaultTagList[i] == 1 and int(gpuFreqList[i]) == 0:
                        print("Tag={} default=yes max_avx512={} max_avx2={} max_power={} min_power={} error_power={} coeffs=coeffs.default powercap=1 powercap_plugin=dvfs.so energy_plugin={} max_powercap={} cpu_max_pstate=0 imc_max_pstate=0 energy_model={} imc_max_freq=0 imc_min_freq=0 idle_governor={} idle_pstate=0".format(typesNameList[i], max_avx512List[i], max_avx2List[i], maxPowerList[i], min_powerList[i], int(maxPowerList[i])+(0.1*int(maxPowerList[i])), energyPluginList[i], int(averagePowerList[i])*int(percentage), energyModelList[i], idle_governorList[i]))
                    if defaultTagList[i] == 1 and int(gpuFreqList[i]) > 0:
                        print("Tag={} default=yes max_avx512={} max_avx2={} max_power={} min_power={} error_power={} coeffs=coeffs.default powercap=1 powercap_plugin=dvfs.so energy_plugin={} gpu_powercap_plugin=gpu.so max_powercap={} gpu_def_freq={} cpu_max_pstate=0 imc_max_pstate=0 energy_model={} imc_max_freq=0 imc_min_freq=0 idle_governor={} idle_pstate=0".format(typesNameList[i], max_avx512List[i], max_avx2List[i], maxPowerList[i], min_powerList[i], int(maxPowerList[i])+(0.1*int(maxPowerList[i])), energyPluginList[i], int(averagePowerList[i])*int(percentage), gpuFreqList[i], energyModelList[i], idle_governorList[i]))
                    if defaultTagList[i] == 0 and int(gpuFreqList[i]) == 0:
                        print("Tag={} max_avx512={} max_avx2={} max_power={} min_power={} error_power={} coeffs=coeffs.default powercap=1 powercap_plugin=dvfs.so energy_plugin={} max_powercap={} cpu_max_pstate=0 imc_max_pstate=0 energy_model={} imc_max_freq=0 imc_min_freq=0 idle_governor={} idle_pstate=0".format(typesNameList[i], max_avx512List[i], max_avx2List[i], maxPowerList[i], min_powerList[i], int(maxPowerList[i])+(0.1*int(maxPowerList[i])), energyPluginList[i], int(averagePowerList[i])*int(percentage), energyModelList[i], idle_governorList[i]))
                    if defaultTagList[i] == 0 and int(gpuFreqList[i]) > 0:
                        print("Tag={} max_avx512={} max_avx2={} max_power={} min_power={} error_power={} coeffs=coeffs.default powercap=1 powercap_plugin=dvfs.so energy_plugin={} gpu_powercap_plugin=gpu.so max_powercap={} gpu_def_freq={} cpu_max_pstate=0 imc_max_pstate=0 energy_model={} imc_max_freq=0 imc_min_freq=0 idle_governor={} idle_pstate=0".format(typesNameList[i], max_avx512List[i], max_avx2List[i], maxPowerList[i], min_powerList[i], int(maxPowerList[i])+(0.1*int(maxPowerList[i])), energyPluginList[i], int(averagePowerList[i])*int(percentage), gpuFreqList[i], energyModelList[i], idle_governorList[i]))

        if powerCap in ['y', 'Y'] and EARGMPowerCapMode in ['1'] and powerLimit in ['y', 'Y']:
                for i in range(len(typesNameList)):
                    percentage = ((abs(int(eargmPowerList[i])) - eargmPowerSug)*100)/eargmPowerSug
                    #if int(i) == 0:
                        #i == 'all_types'
                    if defaultTagList[i] == 1 or int(gpuFreqList[i]) == 0:
                        print("Tag={} default=yes max_avx512={} max_avx2={} max_power={} min_power={} error_power={} coeffs=coeffs.default powercap={} powercap_plugin=dvfs.so energy_plugin={} max_powercap={} cpu_max_pstate=0 imc_max_pstate=0 energy_model={} imc_max_freq=0 imc_min_freq=0 idle_governor={} idle_pstate=0".format(typesNameList[i], max_avx512List[i], max_avx2List[i], maxPowerList[i], min_powerList[i], int(maxPowerList[i])+(0.1*int(maxPowerList[i])), int(averagePowerList[i])*int(percentage), energyPluginList[i], int(maxPowerList[i]), energyModelList[i], idle_governorList[i]))
                    if defaultTagList[i] == 1 or int(gpuFreqList[i]) > 0:
                        print("Tag={} default=yes max_avx512={} max_avx2={} max_power={} min_power={} error_power={} coeffs=coeffs.default powercap={} powercap_plugin=dvfs.so energy_plugin={} gpu_powercap_plugin=gpu.so max_powercap={} gpu_def_freq={} cpu_max_pstate=0 imc_max_pstate=0 energy_model={} imc_max_freq=0 imc_min_freq=0 idle_governor={} idle_pstate=0".format(typesNameList[i], max_avx512List[i], max_avx2List[i], maxPowerList[i], min_powerList[i], int(maxPowerList[i])+(0.1*int(maxPowerList[i])), int(averagePowerList[i])*int(percentage), energyPluginList[i], int(maxPowerList[i]), gpuFreqList[i], energyModelList[i], idle_governorList[i]))
                    if defaultTagList[i] == 0 and int(gpuFreqList[i]) == 0:
                        print("Tag={} max_avx512={} max_avx2={} max_power={} min_power={} error_power={} coeffs=coeffs.default powercap={} powercap_plugin=dvfs.so energy_plugin={} max_powercap={} cpu_max_pstate=0 imc_max_pstate=0 energy_model={} imc_max_freq=0 imc_min_freq=0 idle_governor={} idle_pstate=0".format(typesNameList[i], max_avx512List[i], max_avx2List[i], maxPowerList[i], min_powerList[i], int(maxPowerList[i])+(0.1*int(maxPowerList[i])), int(averagePowerList[i])*int(percentage), energyPluginList[i], int(maxPowerList[i]), energyModelList[i], idle_governorList[i]))
                    if defaultTagList[i] == 0 and int(gpuFreqList[i]) > 0:
                        print("Tag={} max_avx512={} max_avx2={} max_power={} min_power={} error_power={} coeffs=coeffs.default powercap={} powercap_plugin=dvfs.so energy_plugin={} gpu_powercap_plugin=gpu.so max_powercap={} gpu_def_freq={} cpu_max_pstate=0 imc_max_pstate=0 energy_model={} imc_max_freq=0 imc_min_freq=0 idle_governor={} idle_pstate=0".format(typesNameList[i], max_avx512List[i], max_avx2List[i], maxPowerList[i], min_powerList[i], int(maxPowerList[i])+(0.1*int(maxPowerList[i])), int(averagePowerList[i])*int(percentage), energyPluginList[i], int(maxPowerList[i]), gpuFreqList[i], energyModelList[i], idle_governorList[i]))

        if powerCap in ['y', 'Y'] and EARGMPowerCapMode in ['0', '2'] and powerLimit in ['n', 'N']:
                for i in range(len(typesNameList)):
                    #if int(i) == 0:
                        #i == 'all_types'
                    if defaultTagList[i] == 1 and int(gpuFreqList[i]) == 0:
                        print("Tag={} default=yes max_avx512={} max_avx2={} max_power={} min_power={} error_power={} coeffs=coeffs.default powercap=1 powercap_plugin=dvfs.so energy_plugin={} max_powercap={} cpu_max_pstate=0 imc_max_pstate=0 energy_model={} imc_max_freq=0 imc_min_freq=0 idle_governor={} idle_pstate=0".format(typesNameList[i], max_avx512List[i], max_avx2List[i], maxPowerList[i], min_powerList[i], int(maxPowerList[i])+(0.1*int(maxPowerList[i])), energyPluginList[i], int(averagePowerList[i]), energyModelList[i], idle_governorList[i]))
                    if defaultTagList[i] == 1 and int(gpuFreqList[i]) > 0:
                        print("Tag={} default=yes max_avx512={} max_avx2={} max_power={} min_power={} error_power={} coeffs=coeffs.default powercap=1 powercap_plugin=dvfs.so energy_plugin={} gpu_powercap_plugin=gpu.so max_powercap={} gpu_def_freq={} cpu_max_pstate=0 imc_max_pstate=0 energy_model={} imc_max_freq=0 imc_min_freq=0 idle_governor={} idle_pstate=0".format(typesNameList[i], max_avx512List[i], max_avx2List[i], maxPowerList[i], min_powerList[i], int(maxPowerList[i])+(0.1*int(maxPowerList[i])), energyPluginList[i], int(averagePowerList[i]), gpuFreqList[i], energyModelList[i], idle_governorList[i]))
                    if defaultTagList[i] == 0 and int(gpuFreqList[i]) == 0:
                        print("Tag={} max_avx512={} max_avx2={} max_power={} min_power={} error_power={} coeffs=coeffs.default powercap=1 powercap_plugin=dvfs.so energy_plugin={} max_powercap={} cpu_max_pstate=0 imc_max_pstate=0 energy_model={} imc_max_freq=0 imc_min_freq=0 idle_governor={} idle_pstate=0".format(typesNameList[i], max_avx512List[i], max_avx2List[i], maxPowerList[i], min_powerList[i], int(maxPowerList[i])+(0.1*int(maxPowerList[i])), energyPluginList[i], int(averagePowerList[i]), energyModelList[i], idle_governorList[i]))
                    if defaultTagList[i] == 0 and int(gpuFreqList[i]) > 0:
                        print("Tag={} max_avx512={} max_avx2={} max_power={} min_power={} error_power={} coeffs=coeffs.default powercap=1 powercap_plugin=dvfs.so energy_plugin={} gpu_powercap_plugin=gpu.so max_powercap={} gpu_def_freq={} cpu_max_pstate=0 imc_max_pstate=0 energy_model={} imc_max_freq=0 imc_min_freq=0 idle_governor={} idle_pstate=0".format(typesNameList[i], max_avx512List[i], max_avx2List[i], maxPowerList[i], min_powerList[i], int(maxPowerList[i])+(0.1*int(maxPowerList[i])), energyPluginList[i], int(averagePowerList[i]), gpuFreqList[i], energyModelList[i], idle_governorList[i]))

        if powerCap in ['y', 'Y'] and EARGMPowerCapMode in ['1'] and powerLimit in ['n', 'N']:
                for i in range(len(typesNameList)):
                    #if int(i) == 0:
                        #i == 'all_types'
                    if defaultTagList[i] == 1 or int(gpuFreqList[i]) == 0:
                        print("Tag={} default=yes max_avx512={} max_avx2={} max_power={} min_power={} error_power={} coeffs=coeffs.default powercap={} powercap_plugin=dvfs.so energy_plugin={} max_powercap={} cpu_max_pstate=0 imc_max_pstate=0 energy_model={} imc_max_freq=0 imc_min_freq=0 idle_governor={} idle_pstate=0".format(typesNameList[i], max_avx512List[i], max_avx2List[i], maxPowerList[i], min_powerList[i], int(maxPowerList[i])+(0.1*int(maxPowerList[i])), int(averagePowerList[i]), energyPluginList[i], int(maxPowerList[i]), energyModelList[i], idle_governorList[i]))
                    if defaultTagList[i] == 1 or int(gpuFreqList[i]) > 0:
                        print("Tag={} default=yes max_avx512={} max_avx2={} max_power={} min_power={} error_power={} coeffs=coeffs.default powercap={} powercap_plugin=dvfs.so energy_plugin={} gpu_powercap_plugin=gpu.so max_powercap={} gpu_def_freq={} cpu_max_pstate=0 imc_max_pstate=0 energy_model={} imc_max_freq=0 imc_min_freq=0 idle_governor={} idle_pstate=0".format(typesNameList[i], max_avx512List[i], max_avx2List[i], maxPowerList[i], min_powerList[i], int(maxPowerList[i])+(0.1*int(maxPowerList[i])), int(averagePowerList[i]), energyPluginList[i], int(maxPowerList[i]), gpuFreqList[i], energyModelList[i], idle_governorList[i]))
                    if defaultTagList[i] == 0 and int(gpuFreqList[i]) == 0:
                        print("Tag={} max_avx512={} max_avx2={} max_power={} min_power={} error_power={} coeffs=coeffs.default powercap={} powercap_plugin=dvfs.so energy_plugin={} max_powercap={} cpu_max_pstate=0 imc_max_pstate=0 energy_model={} imc_max_freq=0 imc_min_freq=0 idle_governor={} idle_pstate=0".format(typesNameList[i], max_avx512List[i], max_avx2List[i], maxPowerList[i], min_powerList[i], int(maxPowerList[i])+(0.1*int(maxPowerList[i])), int(averagePowerList[i]), energyPluginList[i], int(maxPowerList[i]), energyModelList[i], idle_governorList[i]))
                    if defaultTagList[i] == 0 and int(gpuFreqList[i]) > 0:
                        print("Tag={} max_avx512={} max_avx2={} max_power={} min_power={} error_power={} coeffs=coeffs.default powercap={} powercap_plugin=dvfs.so energy_plugin={} gpu_powercap_plugin=gpu.so max_powercap={} gpu_def_freq={} cpu_max_pstate=0 imc_max_pstate=0 energy_model={} imc_max_freq=0 imc_min_freq=0 idle_governor={} idle_pstate=0".format(typesNameList[i], max_avx512List[i], max_avx2List[i], maxPowerList[i], min_powerList[i], int(maxPowerList[i])+(0.1*int(maxPowerList[i])), int(averagePowerList[i]), energyPluginList[i], int(maxPowerList[i]), gpuFreqList[i], energyModelList[i], idle_governorList[i]))
        print("#")
        print("#---------------------------------------------------------------------------------------------------")
        print("## Power policies")
        print("## ---------------------------------------------------------------------------------------------------")
        print("#")
        print("## Policy names must be exactly file names for policies installeled in the system.")
        print("DefaultPowerPolicy=min_time")
        print("Policy=monitoring Settings= DefaultPstate= Privileged=")
        print("Policy=min_time Settings= DefaultPstate= Privileged=")
        print("Policy=min_energy Settings= DefaultPstate= Privileged=")
        print("#")
        print("# For homogeneous systems, default frequencies can be easily specified using freqs.")
        print("# For heterogeneous systems it is preferred to use pstates or use tags")
        print("#")
        print("")
        print("# Example with freqs (lower pstates corresponds with higher frequencies). Pstate=1 is nominal and 0 is turbo")
        print("#Policy=monitoring Settings=0 DefaultFreq=2.4 Privileged=0")
        print("#Policy=min_time Settings=0.7 DefaultFreq=2.0 Privileged=0")
        print("#Policy=min_energy Settings=0.05 DefaultFreq=2.4 Privileged=1")
        print("")
        print("")
        print("#Example with tags")
        print("#Policy=monitoring Settings=0 DefaultFreq=2.6 Privileged=0 tag=6126")
        print("#Policy=min_time Settings=0.7 DefaultFreq=2.1 Privileged=0 tag=6126")
        print("#Policy=min_energy Settings=0.05 DefaultFreq=2.6 Privileged=1 tag=6126")
        print("#Policy=monitoring Settings=0 DefaultFreq=2.4 Privileged=0 tag=6148")
        print("#Policy=min_time Settings=0.7 DefaultFreq=2.0 Privileged=0 tag=6148")
        print("#Policy=min_energy Settings=0.05 DefaultFreq=2.4 Privileged=1 tag=6148")
        print("")
        print("#---------------------------------------------------------------------------------------------------")
        print("# Energy Tags")
        print("#---------------------------------------------------------------------------------------------------")
        print("#")
        print("# Privileged users, accounts and groups are allowed to use EnergyTags.")
        print("# The \"allowed\" TAGs are defined by row together with the priviledged user/group/account.")
        print("#")
        print("EnergyTag=cpu-intensive pstate=1 users=all")
        print("EnergyTag=turbo pstate=0")
        print("EnergyTag=memory-intensive pstate=4 users=usr1,usr2 groups=grp1,grp2 accounts=acc1,acc2")
        print("")
        print("#---------------------------------------------------------------------------------------------------")
        print("# Node Isles")
        print("#---------------------------------------------------------------------------------------------------")
        print("# It is mandatory to specify all the nodes in the cluster, grouped by islands. More than one line")
        print("# per island must be supported to hold nodes with different names or for pointing to different")
        print("# EARDBDs through its IPs or hostnames.") 
        print("# EARGMID is the field that specifies which EARGM controls the nodes in that line. If no EARGMID")
        print("# is specified, it will pick the first EARGMID value that is found (ie, the previous line's EARGMID).")
        print("#")
        print("#")
        if powerCap in ['n', 'N']:
                for i in range(len(typesNameList)):
                        print("Island={} Nodes=[{}-nodes] Tag={}".format(int(i), typesNameList[i], typesNameList[i]))
        
        if powerCap in ['y', 'Y'] and powerLimit in ['n', 'N']:
                for i in range(len(typesNameList)):
                        print("Island={} Nodes=[{}-nodes] Tag={} EARGMID=1".format(int(i), typesNameList[i], typesNameList[i]))
        
        if powerCap in ['y', 'Y'] and powerLimit in ['y', 'Y']:                
                for i in range(len(eargmIdList)):
                        print("Island={} Nodes=[{}-nodes] Tag={} EARGMID={}".format(int(i), typesNameList[i], typesNameList[i], eargmIdList[i]))
