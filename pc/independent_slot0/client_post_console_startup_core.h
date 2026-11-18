#pragma once
#include "client_object_initializer_core.h"
#include "client_particle_bucket_core.h"
#include "sin_compat/original_subsystem_startup.hpp"

namespace sin_independent {
// Only a caller-owned startup fixture. Not an engine factory or VClient011.
struct ClientVGuiBackend {
    virtual ~ClientVGuiBackend()=default;
    virtual bool CheckModule(std::string_view name) noexcept=0;
    virtual void* Query(std::string_view version) noexcept=0;
    virtual void RegisterOnce() noexcept=0;
    virtual void FirstHelper() noexcept=0;
    virtual bool SecondHelper() noexcept=0;
    // Independent fixture cleanup, NOT an authenticated retail shutdown.
    virtual void ReleaseFixture() noexcept=0;
};
enum class ClientPostConsoleState { Pending, Starting, Ready, Failed, Stopping, Finished };
enum class ClientPostConsoleResult { Ready, AlreadyAttempted, ObjectsFailed, ParticleFailed, VGuiFailed };

template<std::size_t N>
class ClientPostConsoleStartup {
public:
    ClientPostConsoleStartup(ClientInitializers<N>& objects,ClientVGuiBackend& gui,
                             ParticleBucketBackend& particles) noexcept
        : objects_(objects),gui_(gui),bucket_(particles){}
    ~ClientPostConsoleStartup(){ Shutdown(); }
    ClientPostConsoleStartup(const ClientPostConsoleStartup&)=delete;
    ClientPostConsoleStartup& operator=(const ClientPostConsoleStartup&)=delete;
    ClientPostConsoleState State() const noexcept{return state_;}
    const sin_compat::startup::VGuiStartupState& GuiState() const noexcept{return guiState_;}
    const void* Bucket() const noexcept{return bucket_.Bucket();}
    ClientPostConsoleResult Start() noexcept {
        if(state_!=ClientPostConsoleState::Pending)return ClientPostConsoleResult::AlreadyAttempted;
        state_=ClientPostConsoleState::Starting;
        if(objects_.Initialize()!=InitializerResult::Initialized){
            state_=ClientPostConsoleState::Failed;
            return ClientPostConsoleResult::ObjectsFailed;
        }
        objectsOwned_=true;
        if(bucket_.Start()!=ParticleBucketResult::Started)
            return Fail(ClientPostConsoleResult::ParticleFailed);
        guiAttempted_=true;
        using sin_compat::startup::start_client_vgui;
        if(!start_client_vgui(guiState_,
                [this](std::string_view s){return gui_.CheckModule(s);},
                [this](std::string_view s){return gui_.Query(s);},
                [this]{gui_.RegisterOnce();},
                [this]{gui_.FirstHelper();},
                [this]{return gui_.SecondHelper();}))
            return Fail(ClientPostConsoleResult::VGuiFailed);
        state_=ClientPostConsoleState::Ready;
        return ClientPostConsoleResult::Ready;
    }
    bool Shutdown() noexcept {
        if(state_!=ClientPostConsoleState::Ready && state_!=ClientPostConsoleState::Failed)
            return false;
        Cleanup(ClientPostConsoleState::Finished);
        return true;
    }
private:
    ClientPostConsoleResult Fail(ClientPostConsoleResult result) noexcept {
        Cleanup(ClientPostConsoleState::Failed);
        return result;
    }
    void Cleanup(ClientPostConsoleState terminal) noexcept {
        state_=ClientPostConsoleState::Stopping;
        // Chosen ownership order for the independent fixture only.
        if(guiAttempted_){
            guiAttempted_=false;
            gui_.ReleaseFixture();
            guiState_={};
        }
        bucket_.Shutdown();
        if(objectsOwned_){
            objectsOwned_=false;
            objects_.Free();
        }
        state_=terminal;
    }
    ClientInitializers<N>& objects_;
    ClientVGuiBackend& gui_;
    ParticleBucketOwner bucket_;
    sin_compat::startup::VGuiStartupState guiState_{};
    ClientPostConsoleState state_=ClientPostConsoleState::Pending;
    bool objectsOwned_=false,guiAttempted_=false;
};
} // namespace sin_independent
