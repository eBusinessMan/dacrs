// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Dacrs developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "noui.h"

#include "ui_interface.h"
#include "util.h"
#include <stdint.h>
#include <string>
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "json/json_spirit_writer_template.h"
#include "main.h"
#include "rpc/rpctx.h"
#include "wallet/wallet.h"
#include "init.h"
#include "util.h"
using namespace json_spirit;
#include "cuiserver.h"
#include "net.h"
static bool noui_ThreadSafeMessageBox(const std::string& message, const std::string& caption, unsigned int style)
{

	Object obj;
	obj.push_back(Pair("type",     "MessageBox"));
	obj.push_back(Pair("BoxType",     message));

    std::string strCaption;
    // Check for usage of predefined caption
    switch (style) {
    case CClientUIInterface::MSG_ERROR:
      	obj.push_back(Pair("BoxType",     "Error"));
        break;
    case CClientUIInterface::MSG_WARNING:
       	obj.push_back(Pair("BoxType",     "Warning"));
        break;
    case CClientUIInterface::MSG_INFORMATION:
    	obj.push_back(Pair("BoxType",     "Information"));
        break;
    default:
    	obj.push_back(Pair("BoxType",     "unKown"));
//        strCaption += caption; // Use supplied caption (can be empty)
    }

	if (CUIServer::HasConnection()) {
		CUIServer::Send(write_string(Value(std::move(obj)),true));
	} else {
		LogPrint("NOUI", "%s\n", write_string(Value(std::move(obj)),true));
	}

    fprintf(stderr, "%s: %s\n", strCaption.c_str(), message.c_str());
    return false;
}


static bool noui_SyncTx()
{
	Array arrayObj;
	map<uint256, CAccountTx>::iterator iterAccountTx = pwalletMain->mapInBlockTx.begin();
	for(; iterAccountTx != pwalletMain->mapInBlockTx.end(); ++iterAccountTx)
	{
		Object objTx;
		map<uint256, std::shared_ptr<CBaseTransaction> >::iterator iterTx = iterAccountTx->second.mapAccountTx.begin();
		for(;iterTx != iterAccountTx->second.mapAccountTx.end(); ++iterTx) {
			objTx = TxToJSON(iterTx->second.get());
			objTx.push_back(Pair("blockhash", iterAccountTx->first.GetHex()));
			if(mapBlockIndex.count(iterAccountTx->first) && chainActive.Contains(mapBlockIndex[iterAccountTx->first])) {
				objTx.push_back(Pair("confirmHeight", mapBlockIndex[iterAccountTx->first]->nHeight));
				objTx.push_back(Pair("confirmedtime", (int)mapBlockIndex[iterAccountTx->first]->nTime));
			}
			else {
				LogPrint("NOUI", "block hash=%s in wallet map invalid\n", iterAccountTx->first.GetHex());
				continue;
			}
			Object obj;
			obj.push_back(Pair("type",     "SyncTx"));
			obj.push_back(Pair("msg",  objTx));// write_string(Value(arrayObj),true)));
			if(CUIServer::HasConnection()){
				CUIServer::Send(write_string(Value(std::move(obj)),true));
				MilliSleep(10);
			}
			else
			{
				LogPrint("NOUI","init message: %s\n", write_string(Value(std::move(obj)),true));
			}
		}
	}
	map<uint256, std::shared_ptr<CBaseTransaction> >::iterator iterTx =  pwalletMain->UnConfirmTx.begin();
	for(; iterTx != pwalletMain->UnConfirmTx.end(); ++iterTx)
	{
		Object objTx = TxToJSON(iterTx->second.get());
		arrayObj.push_back(objTx);

		Object obj;
		obj.push_back(Pair("type",     "SyncTx"));
		obj.push_back(Pair("msg",   objTx));
		if(CUIServer::HasConnection()){
			CUIServer::Send(write_string(Value(std::move(obj)),true));
			MilliSleep(10);
		}else{
			LogPrint("NOUI","init message: %s\n", write_string(Value(std::move(obj)),true));
		}
	}
	return true;
}
static void noui_InitMessage(const std::string &message)
{
	if(message =="initialize end")
	{
		CUIServer::IsInitalEnd = true;
	}
	if("Sync Tx" == message)
	{
		 noui_SyncTx();
		 return;
	}
	Object obj;
	obj.push_back(Pair("type",     "init"));
	obj.push_back(Pair("msg",     message));
	if(CUIServer::HasConnection()){
		CUIServer::Send(write_string(Value(std::move(obj)),true));
	}else{
		LogPrint("NOUI","init message: %s\n", write_string(Value(std::move(obj)),true));
	}
}

static void noui_BlockChanged(int64_t time,int64_t high,const uint256 &hash) {

	Object obj;
	obj.push_back(Pair("type",     "blockchanged"));
	obj.push_back(Pair("tips",     g_nSyncTipHeight));
	obj.push_back(Pair("high",     (int)high));
	obj.push_back(Pair("hash",     hash.ToString()));
    obj.push_back(Pair("connections",   (int)vNodes.size()));

	if (CUIServer::HasConnection()) {
		CUIServer::Send(write_string(Value(std::move(obj)),true));
	}else
	 {
			LogPrint("NOUI", "%s\n", write_string(Value(std::move(obj)),true));
	}
}

extern Object GetTxDetailJSON(const uint256& txhash);

static bool noui_RevTransaction(const uint256 &hash){
	Object obj;
	obj.push_back(Pair("type",     "revtransaction"));
	obj.push_back(Pair("transation",     GetTxDetailJSON(hash)));

	if (CUIServer::HasConnection()) {
		CUIServer::Send(write_string(Value(std::move(obj)),true));
	} else {
		LogPrint("NOUI", "%s\n", write_string(Value(std::move(obj)),true));
	}
	return true;
}

static bool noui_RevAppTransaction(const CBlock *pBlock ,int nIndex){
	Object obj;
	obj.push_back(Pair("type",     "rev_app_transaction"));
	Object objTx = TxToJSON(pBlock->vptx[nIndex].get());
	objTx.push_back(Pair("blockhash", pBlock->GetHash().GetHex()));
	objTx.push_back(Pair("confirmHeight", (int) pBlock->nHeight));
	objTx.push_back(Pair("confirmedtime", (int) pBlock->nTime));
	obj.push_back(Pair("transation",     objTx));

	if (CUIServer::HasConnection()) {
		CUIServer::Send(write_string(Value(std::move(obj)),true));
	} else {
		LogPrint("NOUI", "%s\n", write_string(Value(std::move(obj)),true));
	}
	return true;
}

static void noui_NotifyMessage(const std::string &message)
{
	Object obj;
	obj.push_back(Pair("type","notify"));
	obj.push_back(Pair("msg",message));
	if(CUIServer::HasConnection()){
		CUIServer::Send(write_string(Value(std::move(obj)),true));
	}else{
		LogPrint("NOUI","init message: %s\n", write_string(Value(std::move(obj)),true));
	}
}

static bool noui_ReleaseTransaction(const uint256 &hash){
	Object obj;
	obj.push_back(Pair("type",     "releasetx"));
	obj.push_back(Pair("hash",   hash.ToString()));

	if (CUIServer::HasConnection()) {
		CUIServer::Send(write_string(Value(std::move(obj)),true));
	} else {
		LogPrint("NOUI", "%s\n", write_string(Value(std::move(obj)),true));
	}
	return true;
}

static bool noui_RemoveTransaction(const uint256 &hash) {
	Object obj;
	obj.push_back(Pair("type",     "rmtx"));
	obj.push_back(Pair("hash",   hash.ToString()));

	if (CUIServer::HasConnection()) {
		CUIServer::Send(write_string(Value(std::move(obj)),true));
	} else {
		LogPrint("NOUI", "%s\n", write_string(Value(std::move(obj)),true));
	}
	return true;
}

void noui_connect()
{
    // Connect Dacrsd signal handlers
	uiInterface.RevTransaction.connect(noui_RevTransaction);
	uiInterface.RevAppTransaction.connect(noui_RevAppTransaction);
    uiInterface.ThreadSafeMessageBox.connect(noui_ThreadSafeMessageBox);
    uiInterface.InitMessage.connect(noui_InitMessage);
    uiInterface.NotifyBlocksChanged.connect(noui_BlockChanged);
    uiInterface.NotifyMessage.connect(noui_NotifyMessage);
    uiInterface.ReleaseTransaction.connect(noui_ReleaseTransaction);
    uiInterface.RemoveTransaction.connect(noui_RemoveTransaction);
}
