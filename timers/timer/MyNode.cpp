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
	_stopThread = true;
}

MyNode::~MyNode()
{
	stop();
}


bool MyNode::init(Flows::PNodeInfo info)
{
	try
	{
		auto settingsIterator = info->info->structValue->find("interval");
		if(settingsIterator != info->info->structValue->end()) _interval = Flows::Math::getNumber(settingsIterator->second->stringValue);

		settingsIterator = info->info->structValue->find("resetafter");
		if(settingsIterator != info->info->structValue->end()) _resetAfter = Flows::Math::getNumber(settingsIterator->second->stringValue);

		if(_interval < 1) _interval = 1;

		_enabled = getNodeData("enabled")->booleanValue;

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
		if(!_enabled) return true;
		std::lock_guard<std::mutex> timerGuard(_timerMutex);
		_stopThread = false;
		if(_timerThread.joinable()) _timerThread.join();
		_timerThread = std::thread(&MyNode::timer, this);

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
		std::lock_guard<std::mutex> timerGuard(_timerMutex);
		_stopThread = true;
		if(_timerThread.joinable()) _timerThread.join();
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

void MyNode::timer()
{
	uint32_t i = 0;
	int32_t sleepingTime = _interval - (Flows::HelperFunctions::getTime() - _inputTime);
	if(sleepingTime < 1) sleepingTime = 1;

	Flows::PVariable message = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
	message->structValue->emplace("payload", std::make_shared<Flows::Variable>(i));

	int64_t startTimeAll = Flows::HelperFunctions::getTime();
	int64_t startTime = Flows::HelperFunctions::getTime();

	while(!_stopThread)
	{
		try
		{
			i++;
			if(sleepingTime > 1000 && sleepingTime < 30000)
			{
				int32_t iterations = sleepingTime / 100;
				for(int32_t j = 0; j < iterations; j++)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					if(_stopThread) break;
				}
				if(sleepingTime % 100) std::this_thread::sleep_for(std::chrono::milliseconds(sleepingTime % 100));
			}
			else if(sleepingTime >= 30000)
			{
				int32_t iterations = sleepingTime / 1000;
				for(int32_t j = 0; j < iterations; j++)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
					if(_stopThread) break;
				}
				if(sleepingTime % 1000) std::this_thread::sleep_for(std::chrono::milliseconds(sleepingTime % 1000));
			}
			else std::this_thread::sleep_for(std::chrono::milliseconds(sleepingTime));
			if(_stopThread) break;
			message->structValue->at("payload")->integerValue = i;
			if(_resetAfter > 0 && Flows::HelperFunctions::getTime() - startTimeAll >= _resetAfter)
			{
				_stopThread = true;
				_enabled = false;
				Flows::PVariable message = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
				message->structValue->emplace("payload", std::make_shared<Flows::Variable>(true));
				output(1, message);
				setNodeData("enabled", std::make_shared<Flows::Variable>(_enabled));
				Flows::PVariable status = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
				status->structValue->emplace("text", std::make_shared<Flows::Variable>("disabled"));
				nodeEvent("statusTop/" + _id, status);
				break;
			}
			output(0, message);
			int64_t diff = Flows::HelperFunctions::getTime() - startTime;
			if(diff <= _interval) sleepingTime = _interval;
			else sleepingTime = _interval - (diff - _interval);
			if(sleepingTime < 1) sleepingTime = 1;
			startTime = Flows::HelperFunctions::getTime();
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
}

void MyNode::input(Flows::PNodeInfo info, uint32_t index, Flows::PVariable message)
{
	try
	{
		_inputTime = Flows::HelperFunctions::getTime();
		_enabled = message->structValue->at("payload")->booleanValue;
		setNodeData("enabled", std::make_shared<Flows::Variable>(_enabled));
		Flows::PVariable status = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
		status->structValue->emplace("text", std::make_shared<Flows::Variable>(_enabled ? "enabled" : "disabled"));
		nodeEvent("statusTop/" + _id, status);
		std::lock_guard<std::mutex> timerGuard(_timerMutex);
		if(_enabled)
		{
			_stopThread = true;
			if(_timerThread.joinable()) _timerThread.join();
			_stopThread = false;
			_timerThread = std::thread(&MyNode::timer, this);
		}
		else
		{
			_stopThread = true;
			if(_timerThread.joinable()) _timerThread.join();
			Flows::PVariable message = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
			message->structValue->emplace("payload", std::make_shared<Flows::Variable>(true));
			output(1, message);
		}
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

}