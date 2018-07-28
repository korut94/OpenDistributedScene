#include <opendht.h>

#include <iostream>
#include <map>
#include <string>

#define DEFAULT_HOST_PORT 4222

#define PLAYER_TABLE_KEY "players"

extern "C"
{
	bool PlayerTableUpdate(const std::vector<std::shared_ptr<dht::Value>>& values, bool expired);
	bool PlayerUpdate(const std::vector<std::shared_ptr<dht::Value>>& attributes);

	typedef int (*OnConnectionStatusChanged)(dht::NodeStatus, dht::NodeStatus);
	typedef int (*OnDone)(bool);
	typedef int (*OnPlayerLeft)(const char *id);
	typedef int (*OnPlayerJoin)(const char *id);

	dht::DhtRunner _node;
	std::map<dht::InfoHash, std::future<size_t>> _playersToken;

	OnPlayerLeft	_playerLeftHandler = nullptr;
	OnPlayerJoin	_playerJoinHandler = nullptr;

	std::future<size_t> _playersTableToken;

	void DHTNodeBootstrap(const char *host, const char *service, OnConnectionStatusChanged callback) {
		std::cout << "Bootstrapping to '" << host << ":" << service << "'" << std::endl;
		_node.bootstrap(host, service);
	}

	void DHTNodePutValue(const char *netId, uint id, const char *serializedValue, int size) {
		std::cout << "Put operation on: " << netId << " " << id << " " << serializedValue << std::endl;

		(void) netId; // For future works

		dht::Value value((const uint8_t*)serializedValue, size);
		// Replace the old existing value over the DHT
		value.id = id;

		_node.putSigned(_node.getNodeId(), std::move(value));
	}

	const char* DHTNodeRun(uint port = DEFAULT_HOST_PORT) {
		_node.run(port, dht::crypto::generateIdentity(), true);
		std::cout << "Node " << _node.getNodeId() << " running on port " << port << "..." << std::endl;
		return _node.getNodeId().to_c_str();
	}

	void DHTNodeSetOnPlayerLeft(OnPlayerLeft handler) {
		_playerLeftHandler = handler;
	}

	void DHTNodeSetOnPlayerJoin(OnPlayerJoin handler) {
		_playerJoinHandler = handler;
	}

	void DHTNodeSetOnStatusChanged(OnConnectionStatusChanged handler) {
		_node.setOnStatusChanged([handler](dht::NodeStatus ipv4, dht::NodeStatus ipv6) {
			std::cout << "Connection status ipv4 changed to: " << ((ipv4 == dht::NodeStatus::Connected) ? "Connected" : "Disconnected") << std::endl;
			std::cout << "Connection status ipv6 changed to: " << ((ipv6 == dht::NodeStatus::Connected) ? "Connected" : "Disconnected") << std::endl;
			if (handler != nullptr) { handler(ipv4, ipv6); }
		});
	}

	void DHTNodeSubscribePlayer(OnDone callback) {
		auto playersTableHash = dht::InfoHash::get(PLAYER_TABLE_KEY);

		_playersTableToken = _node.listen(playersTableHash, PlayerTableUpdate);
		std::cout << "Starting listening '" << PLAYER_TABLE_KEY << "' table to new incoming players connections" << std::endl;
		
		std::cout << "Subscribing node '" << _node.getNodeId() << "' to '" << PLAYER_TABLE_KEY << "' table" << std::endl;
		// Require manual update subscription to avoid expiring
		_node.put(playersTableHash, _node.getNodeId(), callback);
	}

	bool PlayerTableUpdate(const std::vector<std::shared_ptr<dht::Value>> &values, bool expired) {
		for (const auto& v : values) {
			dht::InfoHash hash = dht::Value::unpack<dht::InfoHash>(*v);
			// A different player is joined or expired to the network
			if (hash != _node.getNodeId()) {
				if (expired) {
					if (_playerLeftHandler != nullptr) { _playerLeftHandler(hash.to_c_str()); }
				} else {
					// Start to listen the key player 
					_playersToken[hash] = _node.listen(hash, PlayerUpdate);
					std::cout << "Starting listening player '" << hash << "'" << std::endl;
					if (_playerJoinHandler != nullptr) { _playerJoinHandler(hash.to_c_str()); }
				}
			}

            std::cout << "Player id: " << dht::Value::unpack<dht::InfoHash>(*v) << (expired ? " [expired]" : "") << std::endl;
		}

        return true;
	}

	bool PlayerUpdate(const std::vector<std::shared_ptr<dht::Value>>& attributes) {
		return true;
	}
}
