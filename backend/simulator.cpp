#include "simulator.h"

#include <algorithm>
#include <iostream>

#include "decoder.h"
#include "device_regs.h"

using namespace lc3::core;

static constexpr uint64_t INST_TIMESTEP = 20;

Simulator::Simulator(lc3::utils::IPrinter & printer, lc3::utils::IInputter & inputter, uint32_t print_level) :
    time(0), logger(printer, print_level), inputter(inputter)
{
    devices.emplace_back(std::make_shared<KeyboardDevice>(inputter));
    devices.emplace_back(std::make_shared<DisplayDevice>(logger));

    for(PIDevice dev : devices) {
        for(uint16_t dev_addr : dev->getAddrMap()) {
            state.registerDeviceReg(dev_addr, dev);
        }
    }
}

void Simulator::simulate(void)
{
    sim::Decoder decoder;
    events.emplace(std::make_shared<StartupEvent>(time));

    for(PIDevice dev : devices) {
        dev->startup();
    }

    do {
        uint64_t fetch_time_offset = INST_TIMESTEP - (time % INST_TIMESTEP);

        auto bp_search = std::find(breakpoints.begin(), breakpoints.end(), state.readPC());

        // Either insert breakpoints event or normal processing.
        if(bp_search != breakpoints.end()) {
            state.writeMCR(state.readMCR() & 0x7FFF);
            events.emplace(std::make_shared<CallbackEvent>(
                time + fetch_time_offset + callbackTypeToUnderlying(CallbackType::BREAKPOINT), CallbackType::BREAKPOINT,
                std::bind(callbackDispatcher, this, CallbackType::BREAKPOINT, std::placeholders::_2)
            ));
        } else {
            // Insert pre- and post-instruction callback events.
            events.emplace(std::make_shared<CallbackEvent>(
                time + fetch_time_offset + callbackTypeToUnderlying(CallbackType::PRE_INST), CallbackType::PRE_INST,
                std::bind(callbackDispatcher, this, CallbackType::PRE_INST, std::placeholders::_2)
            ));
            events.emplace(std::make_shared<CallbackEvent>(
                time + fetch_time_offset + callbackTypeToUnderlying(CallbackType::POST_INST), CallbackType::POST_INST,
                std::bind(callbackDispatcher, this, CallbackType::POST_INST, std::placeholders::_2)
            ));

            // Insert device update events.
            for(PIDevice dev : devices) {
                events.emplace(std::make_shared<DeviceUpdateEvent>(time + fetch_time_offset - 10, dev));
            }

            // Insert instruction fetch event.
            events.emplace(std::make_shared<AtomicInstProcessEvent>(time + fetch_time_offset, decoder));
        }

        // Execute events.
        mainLoop();

        // Insert callback events that might have been generated during execution.
        for(CallbackType cb : state.getPendingCallbacks()) {
            events.emplace(std::make_shared<CallbackEvent>(
                time + callbackTypeToUnderlying(cb), cb, std::bind(callbackDispatcher, this, cb, std::placeholders::_2)
            ));
        }
        state.clearPendingCallbacks();
    } while(lc3::utils::getBit(state.readMCR(), 15) == 1);

    for(PIDevice dev : devices) {
        dev->shutdown();
    }
}

void Simulator::loadObjFile(std::string filename, std::istream & buffer)
{
    events.emplace(std::make_shared<LoadObjFileEvent>(time + 1, filename, buffer, logger));

    mainLoop();
}

void Simulator::registerCallback(CallbackType type, Callback func)
{
    callbacks[type] = func;
}

void Simulator::addBreakpoint(uint16_t pc)
{
    breakpoints.insert(pc);
}

void Simulator::mainLoop(void)
{
    while(! events.empty()) {
        PIEvent event = events.top();
        events.pop();

        if(event != nullptr) {
            if(event->time < time) {
                logger.printf(lc3::utils::PrintType::P_WARNING, true, "%d: Skipping '%s' scheduled for %d", time,
                    event->toString(state).c_str(), event->time);
                continue;
            }

            time = event->time;
            logger.printf(lc3::utils::PrintType::P_EXTRA, true, "%d: %s", time, event->toString(state).c_str());
            event->handleEvent(state);

            PIMicroOp uop = event->uops;
            while(uop != nullptr) {
                logger.printf(lc3::utils::PrintType::P_EXTRA, true, "%d: |- %s", time, uop->toString(state).c_str());
                uop->handleMicroOp(state);
                uop = uop->getNext();
            }
        }

        logger.newline(lc3::utils::PrintType::P_EXTRA);
    }
}

void Simulator::callbackDispatcher(Simulator * sim, CallbackType type, MachineState & state)
{
    auto search = sim->callbacks.find(type);
    if(search != sim->callbacks.end()) {
        search->second(state);
    }
}
