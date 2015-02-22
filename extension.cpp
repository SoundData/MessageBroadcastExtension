/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Sample Extension
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#include "extension.h"

/**
 * @file extension.cpp
 * @brief Implementation of MessageBroadcaster extension.
 */

MessageBroadcaster	g_MessageBroadcaster;			// Global singleton for extension's main interface */
zmq::context_t*		MessageBroadcaster::context;	// Static field initializer
zmq::socket_t*		MessageBroadcaster::socket;		// Static field initializer
cell_t				TellClientAbout(IPluginContext *pContext, const cell_t *params);
void				InitializeZMQ();

// Bind list for SourcePawn native functions
// SourcePawn uses this structure to bind the string name representation of a native
// function in SourcePawn to the C++ implementation at runtime. Terminated with NULL-bind
const sp_nativeinfo_t MyNatives[] = {
		{ "TellClientAbout"	, TellClientAbout },
		{ NULL				, NULL }
};

cell_t TellClientAbout(IPluginContext *pContext, const cell_t *params)
{
	// This function should only ever be called after SDK_OnLoad has handled 0MQ initialization
	assert(MessageBroadcaster::context != NULL);
	assert(MessageBroadcaster::socket != NULL);

	// Get message from SourcePawn string
	char *eventInfo;
	pContext->LocalToString(params[1], &eventInfo);

	// Verify the maximum message size is not exceeded
	const size_t strSz = strnlen_s(eventInfo, MAX_MESSAGE_SIZE);
	// strlen_s will return a size strictly less than MAX_MESSAGE_SIZE
	// if the original string was properly null-terminated
	assert(strSz < MAX_MESSAGE_SIZE);

	// Form the ZeroMQ message
	zmq::message_t msg(strSz + 1);
	strncpy(reinterpret_cast<char*>(msg.data()), eventInfo, strSz);
	// terminate the string
	*(reinterpret_cast<char*>(msg.data()) + strSz) = '\0';

	// Send message to subscribed client(s)
	MessageBroadcaster::socket->send(msg);

	return 0;
}

bool MessageBroadcaster::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	sharesys->AddNatives(myself, MyNatives);
	InitializeZMQ();
	return true;
}

void InitializeZMQ()
{
	// Perform ZMQ init, establish a context and bind a publisher socket
#define SOCK MessageBroadcaster::socket
#define CTX MessageBroadcaster::context
	CTX = new zmq::context_t(1);
	SOCK = new zmq::socket_t(*CTX, ZMQ_PUB);
	assert(CTX != NULL);
	assert(SOCK != NULL);
	SOCK->bind("tcp://*:" ZMQ_PUB_PORT);
#undef SOCK
#undef CTX
}

SMEXT_LINK(&g_MessageBroadcaster);
