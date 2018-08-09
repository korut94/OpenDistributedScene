#include <opendht.h>

#include <iostream>
#include <string>

#define DEFAULT_HOST_PORT 4222

#define PLAYER_TABLE_KEY "players"

extern "C"
{
	bool PlayerTableUpdate(const std::vector<std::shared_ptr<dht::Value>>& values, bool expired);

	typedef int (*OnConnectionStatusChanged)(dht::NodeStatus, dht::NodeStatus);
	typedef int (*OnDone)(bool done);
	typedef int (*OnPlayerLeft)(const char *id);
	typedef int (*OnPlayerJoin)(const char *id);
	typedef int (*OnValueChanged)(const char *entity, uint id, const char *value);

	dht::DhtRunner _node;
	std::vector<dht::InfoHash> _playersHash;

	OnPlayerLeft	_playerLeftHandler = nullptr;
	OnPlayerJoin	_playerJoinHandler = nullptr;
	OnValueChanged	_valueChangedHandler = nullptr;

	std::future<size_t> _playersTableToken;

	void DHTNodeBootstrap(const char *host, const char *service, OnConnectionStatusChanged callback) {
		std::cout << "Bootstrapping to '" << host << ":" << service << "'" << std::endl;
		_node.bootstrap(host, service);

		// Prefetch all the players already connected
		_node.get(dht::InfoHash::get(PLAYER_TABLE_KEY), [](const std::vector<std::shared_ptr<dht::Value>> &values) {
			PlayerTableUpdate(values, false);
			return true;
		}, [=](bool done) {});
	}

	void DHTNodeFetchPlayersUpdate() {
		for (const auto &player : _playersHash) {
			_node.get(player, [&player](const std::vector<std::shared_ptr<dht::Value>>& attributes) {
				for (const auto& attr : attributes) {
					if (_valueChangedHandler != nullptr) {
						_valueChangedHandler(player.to_c_str(), attr->id, dht::Value::unpack<std::string>(*attr).c_str());
					}
				}
				return true;
			}, [](bool success) {});
		}
	}

	void DHTNodePutValue(const char *netId, uint id, const char *serializedValue) {
		std::cout << "Put operation on: " << netId << " " << id << " " << serializedValue << std::endl;

		(void) netId; // For future works

		std::string buffer(serializedValue);
		dht::Value value(buffer);
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

	void DHTNodeSetOnValueChanged(OnValueChanged handler) {
		_valueChangedHandler = handler;
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
	
		std::cout << "Subscribing node '" << _node.getNodeId() << "' to '" << PLAYER_TABLE_KEY << "' table..." << std::endl;
		_node.put(playersTableHash, _node.getNodeId(), [callback, playersTableHash](bool done) {
			std::cout << "Starting listening '" << PLAYER_TABLE_KEY << "' table to new incoming players connections" << std::endl;
			_playersTableToken = _node.listen(playersTableHash, PlayerTableUpdate);
			callback(done);
		});
	}

	bool PlayerTableUpdate(const std::vector<std::shared_ptr<dht::Value>> &values, bool expired) {
		for (const auto& v : values) {
			dht::InfoHash player = dht::Value::unpack<dht::InfoHash>(*v);
			// A different player is joined or expired to the network
			if (player != _node.getNodeId()) {
				if (expired) {
					std::cout << "Player " << player << "is expired!" << std::endl;
					if (_playerLeftHandler != nullptr) { _playerLeftHandler(player.to_c_str()); }
				} else {
					if (_playerJoinHandler != nullptr) { _playerJoinHandler(player.to_c_str()); }
					std::cout << "Listening player " << player << std::endl;
					// Keeping track of the new player
					// _playersHash.push_back(hash);
					_node.listen(player, [=](const std::vector<std::shared_ptr<dht::Value>>& attributes) {
						for (const auto& attr : attributes) {
							std::cout << "Attribute " << attr->id << " = " << dht::Value::unpack<std::string>(*attr) << std::endl;

							if (_valueChangedHandler != nullptr) {
								_valueChangedHandler(player.to_c_str(), attr->id, dht::Value::unpack<std::string>(*attr).c_str());
							}
						}

						return true;
					});
				}
			}
		}

        return true;
	}
}
