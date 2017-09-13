/* Copyright 2013-2017 Sathya Laufer
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Homegear.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#include "MyNode.h"

namespace MyNode
{

MyNode::MyNode(std::string path, std::string nodeNamespace, std::string type, const std::atomic_bool* frontendConnected) : Flows::INode(path, nodeNamespace, type, frontendConnected)
{
	_bl.reset(new BaseLib::SharedObjects(false));

	_localRpcMethods.emplace("send", std::bind(&MyNode::send, this, std::placeholders::_1));
	_localRpcMethods.emplace("registerNode", std::bind(&MyNode::registerNode, this, std::placeholders::_1));

	std::string authRequiredHeader = "HTTP/1.1 401 Authorization Required\r\nWWW-Authenticate: Basic realm=\"Authentication Required\"\r\nConnection: Keep-Alive\r\nContent-Length: 255\r\n\r\n<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\"><html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\"><title>Authorization Required</title></head><body>Authorization Required</body></html>";
	_authRequiredHeader.insert(_authRequiredHeader.end(), authRequiredHeader.begin(), authRequiredHeader.end());
}

MyNode::~MyNode()
{
}

bool MyNode::init(Flows::PNodeInfo info)
{
	try
	{
		_nodeInfo = info;
		return true;
	}
	catch(const std::exception& ex)
	{
		Flows::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		Flows::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return false;
}

bool MyNode::start()
{
	try
	{
		BaseLib::HttpServer::HttpServerInfo serverInfo;
		serverInfo.packetReceivedCallback = std::bind(&MyNode::packetReceived, this, std::placeholders::_1, std::placeholders::_2);

		std::string listenAddress;
		std::string port;

		auto settingsIterator = _nodeInfo->info->structValue->find("listenaddress");
		if(settingsIterator != _nodeInfo->info->structValue->end()) listenAddress = settingsIterator->second->stringValue;

		if(!listenAddress.empty() && !BaseLib::Net::isIp(listenAddress)) listenAddress = BaseLib::Net::getMyIpAddress(listenAddress);
		else if(listenAddress.empty()) listenAddress = BaseLib::Net::getMyIpAddress();

		settingsIterator = _nodeInfo->info->structValue->find("port");
		if(settingsIterator != _nodeInfo->info->structValue->end()) port = settingsIterator->second->stringValue;

		settingsIterator = _nodeInfo->info->structValue->find("usetls");
		if(settingsIterator != _nodeInfo->info->structValue->end()) serverInfo.useSsl = settingsIterator->second->booleanValue;

		if(serverInfo.useSsl)
		{
			std::string tlsNodeId;
			settingsIterator = _nodeInfo->info->structValue->find("tls");
			if(settingsIterator != _nodeInfo->info->structValue->end()) tlsNodeId = settingsIterator->second->stringValue;

			if(!tlsNodeId.empty())
			{
				serverInfo.certData = getConfigParameter(tlsNodeId, "certdata.password")->stringValue;
				serverInfo.keyData = getConfigParameter(tlsNodeId, "keydata.password")->stringValue;
				serverInfo.dhParamData = getConfigParameter(tlsNodeId, "dhdata.password")->stringValue;
				serverInfo.certFile = getConfigParameter(tlsNodeId, "cert")->stringValue;
				serverInfo.keyFile = getConfigParameter(tlsNodeId, "key")->stringValue;
				serverInfo.dhParamFile = getConfigParameter(tlsNodeId, "dh")->stringValue;
			}
		}

		_username = getNodeData("username")->stringValue;
		_password = getNodeData("password")->stringValue;

		_server.reset(new BaseLib::HttpServer(_bl.get(), serverInfo));
		std::string boundAddress;
		_server->start(listenAddress, port, boundAddress);

		return true;
	}
	catch(const std::exception& ex)
	{
		Flows::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		Flows::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return false;
}

void MyNode::stop()
{
	try
	{
		_server->stop();
	}
	catch(const std::exception& ex)
	{
		Flows::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		Flows::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void MyNode::waitForStop()
{
	try
	{
		_server->waitForStop();
	}
	catch(const std::exception& ex)
	{
		Flows::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		Flows::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

Flows::PVariable MyNode::getConfigParameterIncoming(std::string name)
{
	try
	{
		auto settingsIterator = _nodeInfo->info->structValue->find(name);
		if(settingsIterator != _nodeInfo->info->structValue->end()) return settingsIterator->second;
	}
	catch(const std::exception& ex)
	{
		Flows::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		Flows::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return std::make_shared<Flows::Variable>();
}

BaseLib::TcpSocket::TcpPacket MyNode::getError(int32_t code, std::string longDescription)
{
	std::string codeDescription = _http.getStatusText(code);
	std::string contentString = "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\"><html><head><title>" + std::to_string(code) + " " + codeDescription + "</title></head><body><h1>" + codeDescription + "</h1><p>" + longDescription + "<br/></p></body></html>";
	std::string header;
	std::vector<std::string> additionalHeaders;
	_http.constructHeader(contentString.size(), "text/html", code, codeDescription, additionalHeaders, header);
	BaseLib::TcpSocket::TcpPacket content;
	content.insert(content.end(), header.begin(), header.end());
	content.insert(content.end(), contentString.begin(), contentString.end());
	return content;
}

void MyNode::packetReceived(int32_t clientId, BaseLib::Http http)
{
	try
	{
		if(!_username.empty()) //Basic Auth
		{
			if(http.getHeader().authorization.empty())
			{
				_server->send(clientId, _authRequiredHeader);
				return;
			}

			std::pair<std::string, std::string> authData = BaseLib::HelperFunctions::splitLast(http.getHeader().authorization, ' ');
			BaseLib::HelperFunctions::toLower(authData.first);
			if(authData.first != "basic")
			{
				_server->send(clientId, _authRequiredHeader);
				return;
			}
			std::string decodedData;
			BaseLib::Base64::decode(authData.second, decodedData);
			std::pair<std::string, std::string> credentials = BaseLib::HelperFunctions::splitLast(decodedData, ':');
			BaseLib::HelperFunctions::toLower(credentials.first);
			if(credentials.first != _username || credentials.second != _password)
			{
				_server->send(clientId, _authRequiredHeader);
				return;
			}
		}

		std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
		std::string path = http.getHeader().path;
		auto pathPair = BaseLib::HelperFunctions::splitFirst(path, '?');
		path = pathPair.first;
		pathPair = BaseLib::HelperFunctions::splitFirst(path, ':');
		path = pathPair.first;
		auto nodesIterator = _nodes.find(http.getHeader().path);
		if(nodesIterator == _nodes.end())
		{
			BaseLib::TcpSocket::TcpPacket response = getError(404, "The requested URL " + http.getHeader().path + " was not found on this server.");
			_server->send(clientId, response);
			return;
		}
		auto methodIterator = nodesIterator->second.find(http.getHeader().method);
		if(methodIterator == nodesIterator->second.end())
		{
			BaseLib::TcpSocket::TcpPacket response = getError(404, "The requested URL " + http.getHeader().path + " was not found on this server.");
			_server->send(clientId, response);
			return;
		}

		Flows::PVariable headers = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
		for(auto& header : http.getHeader().fields)
		{
			if(header.first == "authorization") continue;
			headers->structValue->emplace(header.first, std::make_shared<Flows::Variable>(header.second));
		}

		std::string content(http.getContent().data(), http.getContentSize());

		Flows::PArray parameters = std::make_shared<Flows::Array>();
		parameters->reserve(6);
		parameters->push_back(std::make_shared<Flows::Variable>(clientId));
		parameters->push_back(std::make_shared<Flows::Variable>(http.getHeader().path));
		parameters->push_back(std::make_shared<Flows::Variable>(http.getHeader().method));
		parameters->push_back(std::make_shared<Flows::Variable>(http.getHeader().contentType));
		parameters->push_back(headers);
		parameters->push_back(std::make_shared<Flows::Variable>(content));

		invokeNodeMethod(methodIterator->second, "packetReceived", parameters);
	}
	catch(const std::exception& ex)
	{
		Flows::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		Flows::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

std::string MyNode::constructHeader(uint32_t contentLength, int32_t code, Flows::PVariable headers)
{
	std::string header;

	std::string additionalHeaders;
	additionalHeaders.reserve(1024);
	for(auto& header : *headers->arrayValue)
	{
		if(header->stringValue.empty()) continue;
		if(header->stringValue.compare(0, 8, "location") == 0) code = 301;
		additionalHeaders.append(header->stringValue + "\r\n");
	}

	header.reserve(1024);
	header.append("HTTP/1.1 " + std::to_string(code) + " " + _http.getStatusText(code) + "\r\n");
	header.append("Connection: close\r\n");
	header.append(additionalHeaders);
	header.append("Content-Length: ").append(std::to_string(contentLength)).append("\r\n\r\n");

	return header;
}

//{{{ RPC methods
	Flows::PVariable MyNode::send(Flows::PArray parameters)
	{
		try
		{
			if(parameters->size() != 4) return Flows::Variable::createError(-1, "Method expects exactly four parameters. " + std::to_string(parameters->size()) + " given.");
			if(parameters->at(0)->type != Flows::VariableType::tInteger && parameters->at(0)->type != Flows::VariableType::tInteger64) return Flows::Variable::createError(-1, "Parameter 1 is not of type integer.");
			if(parameters->at(1)->type != Flows::VariableType::tInteger && parameters->at(1)->type != Flows::VariableType::tInteger64) return Flows::Variable::createError(-1, "Parameter 2 is not of type integer.");
			if(parameters->at(2)->type != Flows::VariableType::tArray) return Flows::Variable::createError(-1, "Parameter 2 is not of type array.");
			if(parameters->at(3)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 4 is not of type string.");

			std::string header = constructHeader(parameters->at(3)->stringValue.size(), parameters->at(1)->integerValue, parameters->at(2));

			BaseLib::TcpSocket::TcpPacket response;
			response.insert(response.end(), header.begin(), header.end());
			response.insert(response.end(), parameters->at(3)->stringValue.begin(), parameters->at(3)->stringValue.end());

			_server->send(parameters->at(0)->integerValue, response);

			return std::make_shared<Flows::Variable>();
		}
		catch(const std::exception& ex)
		{
			Flows::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		}
		catch(...)
		{
			Flows::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
		return Flows::Variable::createError(-32500, "Unknown application error.");
	}

	Flows::PVariable MyNode::registerNode(Flows::PArray parameters)
	{
		try
		{
			if(parameters->size() != 3) return Flows::Variable::createError(-1, "Method expects exactly 3 parameters. " + std::to_string(parameters->size()) + " given.");
			if(parameters->at(0)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 1 is not of type string.");
			if(parameters->at(1)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 2 is not of type string.");
			if(parameters->at(2)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 3 is not of type string.");

			std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
			_nodes[parameters->at(2)->stringValue][BaseLib::HelperFunctions::toUpper(parameters->at(1)->stringValue)] = parameters->at(0)->stringValue;

			return std::make_shared<Flows::Variable>();
		}
		catch(const std::exception& ex)
		{
			Flows::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		}
		catch(...)
		{
			Flows::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
		return Flows::Variable::createError(-32500, "Unknown application error.");
	}
//}}}

}