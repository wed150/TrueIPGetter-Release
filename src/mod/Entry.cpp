#include "chrono"
#include "optional"

#include "Config.h"
#include "Entry.h"

#include "mc/world/actor/player/Player.h"
#include "mc/world/level/Level.h"
#include "mc/server/commands/CommandOrigin.h"
#include "mc/server/commands/CommandOutput.h"
#include "mc/network/packet/ClientboundCloseFormPacket.h"
#include "mc/network/ServerNetworkHandler.h"

#include "ll/api/data/KeyValueDB.h"
#include "ll/api/form/SimpleForm.h"
#include "ll/api/thread/ThreadPoolExecutor.h"
#include "ll/api/coro/InterruptableSleep.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/player/PlayerJoinEvent.h"
#include "ll/api/command/CommandHandle.h"
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/service/Bedrock.h"
#include "ll/api/utils/RandomUtils.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/Config.h"
#include "ll/api/mod/RegisterHelper.h"
#include "ll/api/coro/CoroTask.h"

#include "httplib.h"
#include "nlohmann/json.hpp"
#include "jwt-cpp/jwt.h"
#include "jwt-cpp/traits/nlohmann-json/defaults.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace my_mod {
    //Global Vatiable
    namespace {
    ll::event::ListenerPtr playerJoinEventListener;
    std::string secret="wed15";
    Config config;
    std::set<mce::UUID> passedPlayers;
    std::unique_ptr<ll::data::KeyValueDB> IPDb;
    httplib::Server g_server;
    std::atomic<bool> g_running(false);
    }

    //Hooks
    LL_AUTO_TYPE_INSTANCE_HOOK(
        playerIpgetHook,
        HookPriority::Normal,

        Player,
        &Player::getIPAndPort,
        std::string
    ) {
        if (this) {
            // 3. 获取 UUID
            auto uuid = this->getUuid().asString();
            if (IPDb&&IPDb->has(uuid)) {
                auto ipAndPort = origin();
                auto colonPos = ipAndPort.find(":");
                std::string port=":14514";
                if (colonPos != std::string::npos) port = ipAndPort.substr(colonPos );
                auto newIP=IPDb->get(uuid).value();
                if (newIP!="0.0.0.0"){
                return newIP+port;}
            }

        }
        return origin();
    }
    LL_AUTO_TYPE_INSTANCE_HOOK(
        networkIpGetHook,
        HookPriority::Normal,
        NetworkIdentifier,
        &NetworkIdentifier::getIPAndPort,
        std::string,
    ) {
        if (auto networkHandlerOpt = ll::service::getServerNetworkHandler()) {
            //Get ServerPlayer&Cheak nullptr
            if (auto* serverPlayer = networkHandlerOpt.value()._getServerPlayer(*this, SubClientId::PrimaryClient)) {
                //Get UUID
                auto uuid = serverPlayer->getUuid().asString();
                if (IPDb&&IPDb->has(uuid)) {
                    auto ipAndPort = origin();
                    auto colonPos = ipAndPort.find(":");
                    std::string port=":14514";
                    if (colonPos != std::string::npos) port = ipAndPort.substr(colonPos );
                    auto newIP=IPDb->get(uuid).value();
                    if (newIP!="1.1.1.1"){
                        return newIP+port;}
                }
            }
        }
        return origin();
    }
    LL_AUTO_TYPE_INSTANCE_HOOK(
        networkAddressGetHook,
        HookPriority::Normal,
        NetworkIdentifier,
        &NetworkIdentifier::getAddress,
        std::string,
    ) {
        auto uuid=ll::service::getServerNetworkHandler().value()._getServerPlayer(*this,SubClientId::PrimaryClient)->getUuid().asString();
        if (IPDb&&IPDb->has(uuid)) {
            return IPDb->get(uuid).value();
        }
        return origin();
    }


    //jwt functions
    bool verifyToken(std::string const& token) {
        try {
            auto decoded = jwt::decode(token);
            auto verifier = jwt::verify()
                .allow_algorithm(jwt::algorithm::hs256(secret)) // 只允许RS256，并且使用公钥
                .with_issuer("wed15")
                .with_audience("game_client");

            verifier.verify(decoded);
        }
        catch (std::exception e) {
            return false;
        }
        return true;
    }
    ll::io::Logger& logger=Entry::getInstance().getSelf().getLogger();

    //checkFrom
    void SendForm(Player& player, std::string token="",bool warn=false) {
        if (IPDb.get()->get(player.getUuid().asString())=="1.1.1.1"){return;};
        if (token=="") {
            token = jwt::create<jwt::traits::nlohmann_json>()
                .set_issuer("wed15")
                .set_subject(player.getUuid().asString())
                .set_audience("game_client")
                .set_expires_at(std::chrono::system_clock::now() + std::chrono::minutes{5})
                .sign(jwt::algorithm::hs256(secret));
            logger.debug("token:"+token);
        }
        std::string content = "我们正在验证您的真实IP\n请不要着急，不要主动关闭，这只会重新请求，浪费您的时间\n完成收集后页面将自动关闭\n如果您出现多次无法通过情况，请联系管理员，管理员可以使用/skipipcheck 玩家名称 true来配置跳过检测";
        if (warn) {
            content += "\n§4您的检测尚未通过，请勿关闭";
        }

        ll::form::SimpleForm form("IP验证", content);
        std::string url = config.IPQueryServer+"/?token="+token+"&url="
                       + config.ServerIP + ":" + std::to_string(config.ServerPort) ;
        form.appendButton("请等待", url, "url")
            .appendButton("卡住了? 点此重新收集")
            .sendTo(player, [token](Player& player, int selected, ll::form::FormCancelReason reason) {
                if (reason == ModalFormCancelReason::UserBusy) {
                    SendForm(player, token);
                    return;
                }
                switch (selected) {
                case 0:{
                    bool isPassed = passedPlayers.contains(player.getUuid());
                    if (!isPassed) {
                        SendForm(player, token);
                    }
                    if (isPassed) {
                        player.sendMessage("已通过IP获取");
                    }
                    break;
                    }
                case 1: {
                    SendForm(player, token);
                    break;
                }
                case -1: {
                    bool isPassed = passedPlayers.contains(player.getUuid());
                    if (!isPassed) {
                        SendForm(player, token);
                    }
                    if (isPassed) {
                        player.sendMessage("已通过IP获取");
                    }
                    break;
                }
            }
        });
    }

    //command
    struct IPGETOverload {
        CommandSelector<Player> player;
        bool                    action;
    };
    void registCommand() {
        auto& command = ll::command::CommandRegistrar::getInstance(false)
                            .getOrCreateCommand("skipipcheck", "跳过ip验证", CommandPermissionLevel::GameDirectors);
        command.overload<IPGETOverload>().required("player").required("action").execute([](CommandOrigin const& origin, CommandOutput& output, IPGETOverload const& overload) {
            if (auto entity = overload.player.results(origin).data) {
                try {
                    for (auto en : *entity) {
                        if (en) {
                            auto uuid=en->getUuid().asString();
                            if (overload.action) {
                                IPDb.get()->set(uuid,"1.1.1.1");
                                output.success("成功执行跳过IP验证操作");

                            }
                            else {
                                if (IPDb.get()->get(uuid)=="1.1.1.1") {
                                    IPDb.get()->del(uuid);
                                }
                                    ;
                                output.success("成功执行取消跳过IP验证操作");

                            }
                        }

                    }
                } catch (...) {
                }
            }
        });
    }

    std::optional<ll::thread::ThreadPoolExecutor> mHttpThreadPool;
    bool initHttpServer(int const& port = 8080) {
        try {
            g_server.Post("/report_ip", [](const httplib::Request& req, httplib::Response& res) {
                try {
                    // parse json
                    json j = json::parse(req.body);
                    std::string tok=j.at("token");
                    // verifyToken
                    logger.debug("IP Report: " + req.body);
                    if (verifyToken(tok)) {
                        auto decoded = jwt::decode(tok);
                        auto strUuid=decoded.get_subject();
                        mce::UUID uuid = mce::UUID::fromString(strUuid);
                        if (!passedPlayers.contains(uuid)){

                        IPDb->set(decoded.get_subject(), j.at("ip"));
                        passedPlayers.insert(uuid);
                        ClientboundCloseFormPacket closeFromPacket;
                        auto player=ll::service::getLevel()->getPlayer(uuid);
                        player->sendNetworkPacket(closeFromPacket);
                        logger.info(player->getRealName()+j.at("ip").get<std::string>());}
                    }else {
                        logger.error("jwt invalid");
                    }
                    // Success
                    json response;

                    response["status"] = "ok";
                    res.set_header("Access-Control-Allow-Origin", "*");
                    res.set_content(response.dump(), "application/json");

                } catch (const std::exception& e) {
                    logger.error("JSON parse error: " + std::string(e.what()));
                    res.status = 400;
                    res.set_content("Invalid JSON", "text/plain");
                }
            });

            g_running = true;
            mHttpThreadPool.emplace("HttpServerPool", 2);
            mHttpThreadPool->execute([port]() {
                logger.info("Starting HTTP server on port " + std::to_string(port));
                if (!g_server.listen("0.0.0.0", port)) {
                    logger.error("Failed to start HTTP server on port " + std::to_string(port));
                    g_running = false;
                }
            });
            g_server.new_task_queue = []() {
                return new httplib::ThreadPool(2);  // 2thread
            };
            logger.info("HTTP server started on port " + std::to_string(port));
            return true;
        } catch (const std::exception& e) {
            logger.error("Failed to start HTTP server: " + std::string(e.what()));
            return false;
        }
    }

    //stop HTTP server
    void stopHttpServer() {
        if (g_running) {
            g_server.stop();
            // destory thread pool
            if (mHttpThreadPool.has_value()) {
                try {
                    mHttpThreadPool->destroy();
                    mHttpThreadPool.reset();
                    logger.debug("HTTP thread pool destroyed");
                } catch (const std::exception& e) {
                    logger.error("Error destroying thread pool: {}");
                }
            }
            g_running = false;
            logger.info("HTTP server stopped");
        }
    }

    Entry& Entry::getInstance() {
        static Entry instance;
        return instance;
    }



    bool Entry::load() {
        getSelf().getLogger().debug("Loading...");
        const auto& IPDbPath = getSelf().getDataDir() / "ipMapping";
        IPDb                 = std::make_unique<ll::data::KeyValueDB>(IPDbPath);
        const auto& configFilePath = getSelf().getConfigDir() / "config.json";
        if (!ll::config::loadConfig(config, configFilePath)) {
            getSelf().getLogger().warn("Cannot load configurations from {}", configFilePath);
            getSelf().getLogger().info("Saving default configurations");
            if (!ll::config::saveConfig(config, configFilePath)) {
                getSelf().getLogger().error("Cannot save default configurations to {}", configFilePath);
            }
        }

        return true;
    }

    bool Entry::enable() {
        registCommand();
        getSelf().getLogger().debug("Enabling...");

        if (config.RamdomSecret) {
            secret=mce::UUID::random().asString();
            getSelf().getLogger().info("Generate Ramdom JWT Secret: {}", secret);
        }
        else {
            secret=config.ModifiedSecret;
        }
        auto& eventBus = ll::event::EventBus::getInstance();

        playerJoinEventListener = eventBus.emplaceListener<ll::event::PlayerJoinEvent>(
            [this](ll::event::PlayerJoinEvent& event) {
                 try {
                     passedPlayers.erase(event.self().getUuid());

                     SendForm(event.self());
                 } catch (const std::exception& e) {
                     getSelf().getLogger().error("Error generating token: {}", e.what());
                 }
            }
        );

        initHttpServer(config.NatPort);
        return true;
    }

    bool Entry::disable() {
        getSelf().getLogger().debug("Disabling...");
        playerIpgetHook::unhook();
        networkIpGetHook::unhook();
        networkAddressGetHook::unhook();
        stopHttpServer();
        return true;
    }

} // namespace my_mod

LL_REGISTER_MOD(my_mod::Entry, my_mod::Entry::getInstance());
