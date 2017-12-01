//
// Copyright (C) 2001, 2003, 2004 Johnny Lai, Monash University, Melbourne, Australia
// Copyright (C) 2005 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#include <iostream>

#include "inet/applications/simbo/Modulos/BotnetPing/BotnetPing.h"

#include "inet/applications/pingapp/PingPayload_m.h"

#include "inet/common/ModuleAccess.h"
#include "inet/common/lifecycle/NodeOperations.h"
#include "inet/common/lifecycle/NodeStatus.h"
#include "inet/networklayer/common/InterfaceEntry.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/contract/IL3AddressType.h"
#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/networklayer/contract/INetworkProtocolControlInfo.h"
//#include <ceventheap.h>

#ifdef WITH_IPv4
#include "inet/networklayer/ipv4/IPv4InterfaceData.h"
#endif // ifdef WITH_IPv4

#ifdef WITH_IPv6
#include "inet/networklayer/ipv6/IPv6InterfaceData.h"
#endif // ifdef WITH_IPv6

namespace inet {

using std::cout;

Define_Module(BotnetPing);

simsignal_t BotnetPing::rttSignal = registerSignal("rtt");
simsignal_t BotnetPing::numLostSignal = registerSignal("numLost");
simsignal_t BotnetPing::outOfOrderArrivalsSignal = registerSignal("outOfOrderArrivals");
simsignal_t BotnetPing::pingTxSeqSignal = registerSignal("pingTxSeq");
simsignal_t BotnetPing::pingRxSeqSignal = registerSignal("pingRxSeq");

enum PingSelfKinds {
    PING_FIRST_ADDR = 1001,
    PING_CHANGE_ADDR,
    PING_SEND,
    PING_INICIAL
};

BotnetPing::BotnetPing()
{
}

BotnetPing::~BotnetPing()
{
}

void BotnetPing::initialize(int stage)
{
    try{
        cSimpleModule::initialize(stage);

        if (stage == INITSTAGE_LOCAL) {

            packetSize = par("packetSize");
            sendIntervalPar = &par("sendInterval");
            sleepDurationPar = &par("sleepDuration");
            hopLimit = par("hopLimit");
            count = par("count");
            startTime = par("startTime");
            stopTime = par("stopTime");
            if (stopTime >= SIMTIME_ZERO && stopTime < startTime)
                throw cRuntimeError("Invalid startTime/stopTime parameters");
            printPing = par("printPing").boolValue();
            continuous = par("continuous").boolValue();

            // state
            pid = -1;
            lastStart = -1;
            sendSeqNo = expectedReplySeqNo = 0;
            WATCH(sendSeqNo);
            WATCH(expectedReplySeqNo);

            // statistics
            rttStat.setName("pingRTT");
            sentCount = lossCount = outOfOrderArrivalCount = numPongs = 0;
            WATCH(lossCount);
            WATCH(outOfOrderArrivalCount);
            WATCH(numPongs);

            // references
            timer = new cMessage("sendPing2", PING_FIRST_ADDR);


        }
        else if (stage == INITSTAGE_APPLICATION_LAYER) {
            // startup
            nodeStatus = dynamic_cast<NodeStatus *>(findContainingNode(this)->getSubmodule("status"));
            if (isEnabled() && isNodeUp()){
               // startSendingPingRequests(); //#####################################################################################MODIFICADO
            }
        }
    }catch (const std::exception& ba){throw cRuntimeError("BotnetPing::initialize ,Erro:%s",ba.what());}
}

void BotnetPing::parseDestAddressesPar()
{
    try{
        srcAddr = L3AddressResolver().resolve(par("srcAddr"));
        const char *destAddrs = par("destAddr");
        if (!strcmp(destAddrs, "*")) {
            destAddresses = getAllAddresses();
        }
        else {
            cStringTokenizer tokenizer(destAddrs);
            const char *token;

            while ((token = tokenizer.nextToken()) != nullptr) {
                L3Address addr = L3AddressResolver().resolve(token);
                destAddresses.push_back(addr);
            }
        }
    }catch (const std::exception& ba){throw cRuntimeError("BotnetPing::parseDestAddressesPar ,Erro:%s",ba.what());}
}

void BotnetPing::handleMessage(cMessage *msg)
{//Fixme : verificar delete antes dos returns.
    try{
        if (!isNodeUp()) {
            if (msg->isSelfMessage())
                throw cRuntimeError("Self message '%s' received when %s is down", msg->getName(), getComponentType()->getName());
            else {
                EV_WARN << "BotnetPing is down, dropping '" << msg->getName() << "' message\n";
                return;
            }
        }
        if (msg->isSelfMessage()) {
            if (msg->getKind() == PING_FIRST_ADDR) {
                srcAddr = L3AddressResolver().resolve(par("srcAddr"));
                parseDestAddressesPar();
                if (destAddresses.empty()) {
                    return;
                }
                destAddrIdx = 0;
                msg->setKind(PING_CHANGE_ADDR);
            }

            if (msg->getKind() == PING_CHANGE_ADDR) {
                if (destAddrIdx >= destAddresses.size()){ delete msg; return; }

                destAddr = destAddresses[destAddrIdx];
                EV_INFO << "Starting up: dest=" << destAddr << "  src=" << srcAddr << "seqNo=" << sendSeqNo << endl;
                ASSERT(!destAddr.isUnspecified());
                msg->setKind(PING_SEND);
            }

            ASSERT2(msg->getKind() == PING_SEND, "Unknown kind in self message.");

            // send a ping
            terminado = false;//#####################################################################################MODIFICADO
            sendPing();

            if (count > 0 && sendSeqNo % count == 0) {
                // choose next dest address
                destAddrIdx++;
                msg->setKind(PING_CHANGE_ADDR);
                if (destAddrIdx >= destAddresses.size()) {
                    if (continuous) {
                        destAddrIdx = destAddrIdx % destAddresses.size();
                    }
                }
            }

            // then schedule next one if needed
            scheduleNextPingRequest(simTime(), msg->getKind() == PING_CHANGE_ADDR);
        }
        else {
            // process ping response
            processPingResponse(check_and_cast<PingPayload *>(msg));

        }

        if (hasGUI()) {
            char buf[40];
            sprintf(buf, "sent: %ld pks\nrcvd: %ld pks", sentCount, numPongs);
            getDisplayString().setTagArg("t", 0, buf);
        }
    }catch (const std::exception& ba){throw cRuntimeError("BotnetPing::handleMessage ,Erro:%s",ba.what());}
}

bool BotnetPing::handleOperationStage(LifecycleOperation *operation, int stage, IDoneCallback *doneCallback)
{
    try{
        Enter_Method_Silent();
        if (dynamic_cast<NodeStartOperation *>(operation)) {
            if ((NodeStartOperation::Stage)stage == NodeStartOperation::STAGE_APPLICATION_LAYER && isEnabled())
                startSendingPingRequests();
        }
        else if (dynamic_cast<NodeShutdownOperation *>(operation)) {
            if ((NodeShutdownOperation::Stage)stage == NodeShutdownOperation::STAGE_APPLICATION_LAYER)
                stopSendingPingRequests();
        }
        else if (dynamic_cast<NodeCrashOperation *>(operation)) {
            if ((NodeCrashOperation::Stage)stage == NodeCrashOperation::STAGE_CRASH)
                stopSendingPingRequests();
        }
        else
            throw cRuntimeError("Unsupported lifecycle operation '%s'", operation->getClassName());
        return true;
    }catch (const std::exception& ba){throw cRuntimeError("BotnetPing::handleOperationStage ,Erro:%s",ba.what());}
}
/////////////////////////////////////////////////////////////////////////////
//Todo: aqui
void BotnetPing::startSendingPingRequests()
{
    try{
        Enter_Method_Silent();
        ASSERT(!timer->isScheduled());
        pid = getSimulation()->getUniqueNumber();
        lastStart = simTime();
        timer->setKind(PING_FIRST_ADDR);
        sentCount = 0;
        sendSeqNo = 0;
        scheduleNextPingRequest(-1, false);
    }catch (const std::exception& ba){throw cRuntimeError("BotnetPing::startSendingPingRequests ,Erro:%s",ba.what());}
}

void BotnetPing::stopSendingPingRequests()
{
    try{
        //Enter_Method_Silent();
        pid = -1;
        lastStart = -1;
        sendSeqNo = expectedReplySeqNo = 0;
        srcAddr = destAddr = L3Address();
        destAddresses.clear();
        destAddrIdx = -1;
        cancelNextPingRequest();
    }catch (const std::exception& ba){throw cRuntimeError("BotnetPing::stopSendingPingRequests ,Erro:%s",ba.what());}
}

void BotnetPing::scheduleNextPingRequest(simtime_t previous, bool withSleep)
{
    try{
        simtime_t next;
        if (previous < SIMTIME_ZERO)
            next = simTime() <= startTime ? startTime : simTime();
        else {
            next = previous + sendIntervalPar->doubleValue();
            if (withSleep)
                next += sleepDurationPar->doubleValue();
        }
        if (stopTime < SIMTIME_ZERO || next < stopTime)
            scheduleAt(next, timer);
    }catch (const std::exception& ba){throw cRuntimeError("BotnetPing::scheduleNextPingRequest ,Erro:%s",ba.what());}
}

void BotnetPing::cancelNextPingRequest()
{
    try{
        //Enter_Method_Silent();
        //printf("%s\n", timer->str());
        cancelEvent(timer);
        //delete timer;
        //cancelAndDelete(timer);
    }catch (const std::exception& ba){throw cRuntimeError("BotnetPing::cancelNextPingRequest ,Erro:%s",ba.what());}
}

bool BotnetPing::isNodeUp()
{
    try{
    return !nodeStatus || nodeStatus->getState() == NodeStatus::UP;
    }catch (const std::exception& ba){throw cRuntimeError("BotnetPing::isNodeUp ,Erro:%s",ba.what());}
}

bool BotnetPing::isEnabled()
{
    try{
        return par("destAddr").stringValue()[0] && (count == -1 || sentCount < count);
    }catch (const std::exception& ba){throw cRuntimeError("BotnetPing::isEnabled ,Erro:%s",ba.what());}
}

void BotnetPing::processPingResponse(PingPayload *msg)
{
    try{
        if (msg->getOriginatorId() != pid) {
            EV_WARN << "Received response was not sent by this application, dropping packet\n";
            delete msg;
            return;
        }

        // get src, hopCount etc from packet, and print them
        INetworkProtocolControlInfo *ctrl = check_and_cast<INetworkProtocolControlInfo *>(msg->getControlInfo());
        L3Address src = ctrl->getSourceAddress();
        //L3Address dest = ctrl->getDestinationAddress();
        int msgHopCount = ctrl->getHopLimit();

        // calculate the RTT time by looking up the the send time of the packet
        // if the send time is no longer available (i.e. the packet is very old and the
        // sendTime was overwritten in the circular buffer) then we just return a 0
        // to signal that this value should not be used during the RTT statistics)
        simtime_t rtt = sendSeqNo - msg->getSeqNo() > PING_HISTORY_SIZE ?
            0 : simTime() - sendTimeHistory[msg->getSeqNo() % PING_HISTORY_SIZE];

        if (printPing) {
            cout << getFullPath() << ": reply of " << std::dec << msg->getByteLength()
                 << " bytes from " << src
                 << " icmp_seq=" << msg->getSeqNo() << " ttl=" << msgHopCount
                 << " time=" << (rtt * 1000) << " msec"
                 << " (" << msg->getName() << ")" << endl;
        }

        adicionaEndereco(src);
        printf("Added address: %s\n", src.str().c_str());

        // update statistics
        countPingResponse(msg->getByteLength(), msg->getSeqNo(), rtt);
        delete msg;
    }catch (const std::exception& ba){throw cRuntimeError("BotnetPing::processPingResponse ,Erro:%s",ba.what());}
}

//////////////////////////////Minhas Fun��es //////////////////////////////


void BotnetPing::adicionaEndereco(L3Address src){
    try{
        enderecos.push_back(src);
    }catch (const std::exception& ba){throw cRuntimeError("BotnetPing::adicionaEndereco ,Erro:%s",ba.what());}
}

bool BotnetPing::pingFinalizado(){
    try{
        if (destAddresses.size() == (uint)sentCount){
            Enter_Method_Silent();
            cancelNextPingRequest();
            return true;
        }else{
            return false;
        }
    }catch (const std::exception& ba){throw cRuntimeError("BotnetPing::pingFinalizado ,Erro:%s",ba.what());}
}

std::vector<L3Address> BotnetPing::carregaEnderecos(){
    try{
        return enderecos;
    }catch (const std::exception& ba){throw cRuntimeError("BotnetPing::carregaEnderecos ,Erro:%s",ba.what());}
}




////////////////////////////////////////////////////////////////////////////



void BotnetPing::countPingResponse(int bytes, long seqNo, simtime_t rtt)
{
    try{
        EV_INFO << "Ping reply #" << seqNo << " arrived, rtt=" << rtt << "\n";
        emit(pingRxSeqSignal, seqNo);

        numPongs++;

        // count only non 0 RTT values as 0s are invalid
        if (rtt > 0) {
            rttStat.collect(rtt);
            emit(rttSignal, rtt);
        }

        if (seqNo == expectedReplySeqNo) {
            // expected ping reply arrived; expect next sequence number
            expectedReplySeqNo++;
        }
        else if (seqNo > expectedReplySeqNo) {
            EV_DETAIL << "Jump in seq numbers, assuming pings since #" << expectedReplySeqNo << " got lost\n";

            // jump in the sequence: count pings in gap as lost for now
            // (if they arrive later, we'll decrement back the loss counter)
            long jump = seqNo - expectedReplySeqNo;
            lossCount += jump;
            emit(numLostSignal, lossCount);

            // expect sequence numbers to continue from here
            expectedReplySeqNo = seqNo + 1;
        }
        else {    // seqNo < expectedReplySeqNo
                  // ping reply arrived too late: count as out-of-order arrival (not loss after all)
            EV_DETAIL << "Arrived out of order (too late)\n";
            outOfOrderArrivalCount++;
            lossCount--;
            emit(outOfOrderArrivalsSignal, outOfOrderArrivalCount);
            emit(numLostSignal, lossCount);
        }
    }catch (const std::exception& ba){throw cRuntimeError("BotnetPing::countPingResponse ,Erro:%s",ba.what());}
}

std::vector<L3Address> BotnetPing::getAllAddresses()
{
    try{
        std::vector<L3Address> result;

        #if OMNETPP_VERSION < 0x500
            int lastId = getSimulation()->getLastModuleId();
        #else // if OMNETPP_VERSION < 0x500
            int lastId = getSimulation()->getLastComponentId();
        #endif // if OMNETPP_VERSION < 0x500

            for (int i = 0; i <= lastId; i++)
            {
                IInterfaceTable *ift = dynamic_cast<IInterfaceTable *>(getSimulation()->getModule(i));
                if (ift) {
                    for (int j = 0; j < ift->getNumInterfaces(); j++) {
                        InterfaceEntry *ie = ift->getInterface(j);
                        if (ie && !ie->isLoopback()) {
        #ifdef WITH_IPv4
                            if (ie->ipv4Data()) {
                                IPv4Address address = ie->ipv4Data()->getIPAddress();
                                if (!address.isUnspecified())
                                    result.push_back(L3Address(address));
                            }
        #endif // ifdef WITH_IPv4
        #ifdef WITH_IPv6
                            if (ie->ipv6Data()) {
                                for (int k = 0; k < ie->ipv6Data()->getNumAddresses(); k++) {
                                    IPv6Address address = ie->ipv6Data()->getAddress(k);
                                    if (!address.isUnspecified() && address.isGlobal())
                                        result.push_back(L3Address(address));
                                }
                            }
        #endif // ifdef WITH_IPv6
                        }
                    }
                }
            }
            return result;
    }catch (const std::exception& ba){throw cRuntimeError("BotnetPing::getAllAddresses ,Erro:%s",ba.what());}
}

void BotnetPing::finish()
{
    try{
        cancelAndDelete(timer);
        if (sendSeqNo == 0) {
            if (printPing)
                EV_DETAIL << getFullPath() << ": No pings sent, skipping recording statistics and printing results.\n";

            return;
        }

        lossCount += sendSeqNo - expectedReplySeqNo;
        // record statistics
        recordScalar("Pings sent", sendSeqNo);
        recordScalar("ping loss rate (%)", 100 * lossCount / (double)sendSeqNo);
        recordScalar("ping out-of-order rate (%)", 100 * outOfOrderArrivalCount / (double)sendSeqNo);

        // print it to stdout as well
        if (printPing) {
            cout << "--------------------------------------------------------" << endl;
            cout << "\t" << getFullPath() << endl;
            cout << "--------------------------------------------------------" << endl;

            cout << "sent: " << sendSeqNo << "   received: " << numPongs << "   loss rate (%): " << (100 * lossCount / (double)sendSeqNo) << endl;
            cout << "round-trip min/avg/max (ms): " << (rttStat.getMin() * 1000.0) << "/"
                 << (rttStat.getMean() * 1000.0) << "/" << (rttStat.getMax() * 1000.0) << endl;
            cout << "stddev (ms): " << (rttStat.getStddev() * 1000.0) << "   variance:" << rttStat.getVariance() << endl;
            cout << "--------------------------------------------------------" << endl;
        }
    }catch (const std::exception& ba){throw cRuntimeError("BotnetPing::finish ,Erro:%s",ba.what());}
}

void BotnetPing::sendPing()
{
    try{
        char name[32];
        sprintf(name, "ping%ld", sendSeqNo);

        PingPayload *msg = new PingPayload(name);
        ASSERT(pid != -1);
        msg->setOriginatorId(pid);
        msg->setSeqNo(sendSeqNo);
        msg->setByteLength(packetSize + 4);

        // store the sending time in a circular buffer so we can compute RTT when the packet returns
        sendTimeHistory[sendSeqNo % PING_HISTORY_SIZE] = simTime();

        emit(pingTxSeqSignal, sendSeqNo);
        sendSeqNo++;
        sentCount++;
        IL3AddressType *addressType = destAddr.getAddressType();
        INetworkProtocolControlInfo *controlInfo = addressType->createNetworkProtocolControlInfo();
        controlInfo->setSourceAddress(srcAddr);
        controlInfo->setDestinationAddress(destAddr);
        controlInfo->setHopLimit(hopLimit);
        // TODO: remove
        controlInfo->setTransportProtocol(1);    // IP_PROT_ICMP);
        msg->setControlInfo(dynamic_cast<cObject *>(controlInfo));
        EV_INFO << "Sending ping request #" << msg->getSeqNo() << " to lower layer.\n";
        send(msg, "pingOut");
    }catch (const std::exception& ba){throw cRuntimeError("BotnetPing::sendPing ,Erro:%s",ba.what());}
}


} // namespace inet

