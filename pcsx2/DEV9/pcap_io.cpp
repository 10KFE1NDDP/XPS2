/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2020  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PrecompiledHeader.h"

#include "common/Assertions.h"
#include <memory>

#ifdef _WIN32
#include "common/RedtapeWindows.h"
#include "common/StringUtil.h"
#include <WinSock2.h>
#include <iphlpapi.h>
#endif

#include <stdio.h>
#include "pcap_io.h"
#include "DEV9.h"
#include "AdapterUtils.h"
#include "net.h"
#ifndef PCAP_NETMASK_UNKNOWN
#define PCAP_NETMASK_UNKNOWN 0xffffffff
#endif

#ifdef _WIN32
#define PCAPPREFIX "\\Device\\NPF_"
#endif

using namespace PacketReader;
using namespace PacketReader::IP;

#ifdef _WIN32
int gettimeofday(struct timeval* tv, void* tz)
{
	unsigned __int64 ns100; /*time since 1 Jan 1601 in 100ns units */

	GetSystemTimeAsFileTime((LPFILETIME)&ns100);
	tv->tv_usec = (long)((ns100 / 10L) % 1000000L);
	tv->tv_sec = (long)((ns100 - 116444736000000000L) / 10000000L);
	return (0);
}
#endif

PCAPAdapter::PCAPAdapter()
	: NetAdapter()
{
	if (!EmuConfig.DEV9.EthEnable)
		return;
#ifdef _WIN32
	if (!load_pcap())
		return;
#endif

#ifdef _WIN32
	std::string pcapAdapter = PCAPPREFIX + EmuConfig.DEV9.EthDevice;
#else
	std::string pcapAdapter = EmuConfig.DEV9.EthDevice;
#endif

	switched = EmuConfig.DEV9.EthApi == Pcsx2Config::DEV9Options::NetApi::PCAP_Switched;

	if (!InitPCAP(pcapAdapter, switched))
	{
		Console.Error("DEV9: Can't open Device '%s'", EmuConfig.DEV9.EthDevice.c_str());
		return;
	}

	AdapterUtils::Adapter adapter;
	AdapterUtils::AdapterBuffer buffer;
	std::optional<MAC_Address> adMAC = std::nullopt;
	const bool foundAdapter = AdapterUtils::GetAdapter(EmuConfig.DEV9.EthDevice, &adapter, &buffer);
	if (foundAdapter)
		adMAC = AdapterUtils::GetAdapterMAC(&adapter);
	else
		Console.Error("DEV9: Failed to get adapter information");

	if (adMAC.has_value())
	{
		hostMAC = adMAC.value();
		MAC_Address newMAC = ps2MAC;

		//Lets take the hosts last 2 bytes to make it unique on Xlink
		newMAC.bytes[5] = hostMAC.bytes[4];
		newMAC.bytes[4] = hostMAC.bytes[5];

		SetMACAddress(&newMAC);
	}
	else if (switched)
		Console.Error("DEV9: Failed to get MAC address for adapter, proceeding with hardcoded MAC address");
	else
	{
		Console.Error("DEV9: Failed to get MAC address for adapter");
		pcap_close(hpcap);
		hpcap = nullptr;
		return;
	}

	if (switched && !SetMACSwitchedFilter(ps2MAC))
	{
		pcap_close(hpcap);
		hpcap = nullptr;
		Console.Error("DEV9: Can't open Device '%s'", EmuConfig.DEV9.EthDevice.c_str());
		return;
	}

	if (foundAdapter)
		InitInternalServer(&adapter);
	else
		InitInternalServer(nullptr);

	InitPCAPDumper();
}
AdapterOptions PCAPAdapter::GetAdapterOptions()
{
	return AdapterOptions::None;
}
bool PCAPAdapter::blocks()
{
	pxAssert(hpcap);
	return blocking;
}
bool PCAPAdapter::isInitialised()
{
	return hpcap != nullptr;
}
//gets a packet.rv :true success
bool PCAPAdapter::recv(NetPacket* pkt)
{
	pxAssert(hpcap);

	if (!blocking && NetAdapter::recv(pkt))
		return true;

	pcap_pkthdr* header;
	const u_char* pkt_data;

	if (pcap_next_ex(hpcap, &header, &pkt_data) > 0)
	{
		if (header->len > sizeof(pkt->buffer))
		{
			Console.Error("DEV9: Dropped jumbo frame of size: %u", header->len);
			return false;
		}

		pxAssert(header->len == header->caplen);

		memcpy(pkt->buffer, pkt_data, header->len);

		if (!switched)
			SetMACBridgedRecv(pkt);

		if (hpcap_dumper)
			pcap_dump((u_char*)hpcap_dumper, header, pkt_data);
	}
	else
		return false;

	if (VerifyPkt(pkt, header->len))
	{
		InspectRecv(pkt);
		return true;
	}
	else
		return false;
}
//sends the packet .rv :true success
bool PCAPAdapter::send(NetPacket* pkt)
{
	pxAssert(hpcap);

	InspectSend(pkt);
	if (NetAdapter::send(pkt))
		return true;

	if (hpcap_dumper)
	{
		pcap_pkthdr ph;
		gettimeofday(&ph.ts, NULL);
		ph.caplen = pkt->size;
		ph.len = pkt->size;
		pcap_dump((u_char*)hpcap_dumper, &ph, (u_char*)pkt->buffer);
	}

	// TODO: loopback broadcast packets to host pc in switched mode.
	if (!switched)
		SetMACBridgedSend(pkt);

	if (pcap_sendpacket(hpcap, (u_char*)pkt->buffer, pkt->size))
		return false;
	else
		return true;
}

void PCAPAdapter::reloadSettings()
{
	AdapterUtils::Adapter adapter;
	AdapterUtils::AdapterBuffer buffer;
	if (AdapterUtils::GetAdapter(EmuConfig.DEV9.EthDevice, &adapter, &buffer))
		ReloadInternalServer(&adapter);
	else
		ReloadInternalServer(nullptr);
}

PCAPAdapter::~PCAPAdapter()
{
	if (hpcap_dumper)
	{
		pcap_dump_close(hpcap_dumper);
		hpcap_dumper = nullptr;
	}
	if (hpcap)
	{
		pcap_close(hpcap);
		hpcap = nullptr;
	}
}

std::vector<AdapterEntry> PCAPAdapter::GetAdapters()
{
	std::vector<AdapterEntry> nic;

#ifdef _WIN32
	if (!load_pcap())
		return nic;
#endif

	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_if_t* alldevs;
	pcap_if_t* d;

	if (pcap_findalldevs(&alldevs, errbuf) == -1)
		return nic;

	d = alldevs;
	while (d != NULL)
	{
		AdapterEntry entry;
		entry.type = Pcsx2Config::DEV9Options::NetApi::PCAP_Switched;
#ifdef _WIN32
		//guid
		if (!StringUtil::StartsWith(d->name, PCAPPREFIX))
		{
			Console.Error("PCAP: Unexpected Device: ", d->name);
			d = d->next;
			continue;
		}

		entry.guid = std::string(&d->name[strlen(PCAPPREFIX)]);

		IP_ADAPTER_ADDRESSES adapterInfo;
		std::unique_ptr<IP_ADAPTER_ADDRESSES[]> buffer;

		if (AdapterUtils::GetAdapter(entry.guid, &adapterInfo, &buffer))
			entry.name = StringUtil::WideStringToUTF8String(std::wstring(adapterInfo.FriendlyName));
		else
		{
			//have to use description
			//NPCAP 1.10 is using an version of pcap that dosn't
			//allow us to set it to use UTF8
			//see https://github.com/nmap/npcap/issues/276
			//We have to convert from ANSI to wstring, to then convert to UTF8
			const int len_desc = strlen(d->description) + 1;
			const int len_buf = MultiByteToWideChar(CP_ACP, 0, d->description, len_desc, nullptr, 0);

			std::unique_ptr<wchar_t[]> buf = std::make_unique<wchar_t[]>(len_buf);
			MultiByteToWideChar(CP_ACP, 0, d->description, len_desc, buf.get(), len_buf);

			entry.name = StringUtil::WideStringToUTF8String(std::wstring(buf.get()));
		}
#else
		entry.name = std::string(d->name);
		entry.guid = std::string(d->name);
#endif

		nic.push_back(entry);
		entry.type = Pcsx2Config::DEV9Options::NetApi::PCAP_Bridged;
		nic.push_back(entry);
		d = d->next;
	}

	return nic;
}

// Opens device for capture and sets non-blocking.
bool PCAPAdapter::InitPCAP(const std::string& adapter, bool promiscuous)
{
	char errbuf[PCAP_ERRBUF_SIZE];
	Console.WriteLn("DEV9: Opening adapter '%s'...", adapter.c_str());

	// Open the adapter.
	if ((hpcap = pcap_open_live(adapter.c_str(), // Name of the device.
			 65536, // portion of the packet to capture.
			 // 65536 grants that the whole packet will be captured on all the MACs.
			 promiscuous ? 1 : 0,
			 1, // Read timeout.
			 errbuf // Error buffer.
			 )) == nullptr)
	{
		Console.Error("DEV9: %s", errbuf);
		Console.Error("DEV9: Unable to open the adapter. %s is not supported by pcap", adapter.c_str());
		return false;
	}

	if (pcap_setnonblock(hpcap, 1, errbuf) == -1)
	{
		Console.Error("DEV9: Error setting non-blocking: %s", pcap_geterr(hpcap));
		Console.Error("DEV9: Continuing in blocking mode");
		blocking = true;
	}
	else
		blocking = false;

	// Validate.
	const int dlt = pcap_datalink(hpcap);
	const char* dlt_name = pcap_datalink_val_to_name(dlt);

	Console.Error("DEV9: Device uses DLT %d: %s", dlt, dlt_name);
	switch (dlt)
	{
		case DLT_EN10MB:
			//case DLT_IEEE802_11:
			break;
		default:
			Console.Error("ERROR: Unsupported DataLink Type (%d): %s", dlt, dlt_name);
			pcap_close(hpcap);
			hpcap = nullptr;
			return false;
	}

	Console.WriteLn("DEV9: Adapter Ok.");
	return true;
}

void PCAPAdapter::InitPCAPDumper()
{
#ifdef DEBUG
	const std::string plfile(s_strLogPath + "/pkt_log.pcap");
	hpcap_dumper = pcap_dump_open(hpcap, plfile.c_str());
#endif
}

bool PCAPAdapter::SetMACSwitchedFilter(MAC_Address mac)
{
	bpf_program fp;
	char filter[1024] = "ether broadcast or ether dst ";

	char virtual_mac_str[18];
	sprintf(virtual_mac_str, "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x", mac.bytes[0], mac.bytes[1], mac.bytes[2], mac.bytes[3], mac.bytes[4], mac.bytes[5]);
	strcat(filter, virtual_mac_str);

	if (pcap_compile(hpcap, &fp, filter, 1, PCAP_NETMASK_UNKNOWN) == -1)
	{
		Console.Error("DEV9: Error calling pcap_compile: %s", pcap_geterr(hpcap));
		return false;
	}

	int setFilterRet;
	if ((setFilterRet = pcap_setfilter(hpcap, &fp)) == -1)
		Console.Error("DEV9: Error setting filter: %s", pcap_geterr(hpcap));

	pcap_freecode(&fp);
	return setFilterRet != -1;
}

void PCAPAdapter::SetMACBridgedRecv(NetPacket* pkt)
{
	ethernet_header* eth = (ethernet_header*)pkt->buffer;
	if (eth->protocol == 0x0008) // IP
	{
		// Compare DEST IP in IP with the PS2's IP, if they match, change DEST MAC to ps2MAC.
		const ip_header* iph = ((ip_header*)(pkt->buffer + sizeof(ethernet_header)));

		if (*(IP_Address*)&iph->dst == ps2IP)
			eth->dst = *(mac_address*)&ps2MAC;
	}

	if (eth->protocol == 0x0608) // ARP
	{
		// Compare DEST IP in ARP with the PS2's IP, if they match, DEST MAC to ps2MAC on both ARP and ETH Packet headers.
		arp_packet* aph = ((arp_packet*)(pkt->buffer + sizeof(ethernet_header)));

		if (*(IP_Address*)&aph->p_dst == ps2IP)
		{
			eth->dst = *(mac_address*)&ps2MAC;
			((arp_packet*)(pkt->buffer + sizeof(ethernet_header)))->h_dst = *(mac_address*)&ps2MAC;
		}
	}
}

void PCAPAdapter::SetMACBridgedSend(NetPacket* pkt)
{
	ethernet_header* eth = (ethernet_header*)pkt->buffer;
	if (eth->protocol == 0x0008) // IP
	{
		const ip_address ps2_ip = ((ip_header*)((u8*)pkt->buffer + sizeof(ethernet_header)))->src;
		ps2IP = *(IP_Address*)&ps2_ip;
	}
	if (eth->protocol == 0x0608) // ARP
	{
		const ip_address ps2_ip = ((arp_packet*)((u8*)pkt->buffer + sizeof(ethernet_header)))->p_src;
		ps2IP = *(IP_Address*)&ps2_ip;

		((arp_packet*)(pkt->buffer + sizeof(ethernet_header)))->h_src = *(mac_address*)&hostMAC;
	}
	eth->src = *(mac_address*)&hostMAC;
}
