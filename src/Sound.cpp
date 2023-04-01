#include <stdexcept>
#include "Sound.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

Sound::Sound() {

    // Init miniaudio.
    m_maConfig                      = ma_device_config_init(ma_device_type_playback);
    m_maConfig.playback.format      = ma_format_f32;
    m_maConfig.playback.channels    = CHANNEL_COUNT;
    m_maConfig.sampleRate           = SAMPLE_RATE;
    m_maConfig.dataCallback         = &Sound::dataCallback;
    // Passing a pointer to Sound class instance, so we can access data member in a static function.
    m_maConfig.pUserData            = this;

    if(ma_device_init(nullptr, &m_maConfig, m_maDevice.get()) != MA_SUCCESS)
        throw std::runtime_error("Couldn't initialize sound device!");

    m_maNodeGraphConfig = ma_node_graph_config_init(CHANNEL_COUNT);
    if(ma_node_graph_init(&m_maNodeGraphConfig, nullptr, m_maNodeGraph.get()) != MA_SUCCESS) {
        ma_device_uninit(m_maDevice.get());
        throw std::runtime_error("Couldn't initialize sound node graph!");
    }

    m_maLpfConfig = ma_lpf_node_config_init(CHANNEL_COUNT, SAMPLE_RATE, LPF_CUTOFF_FREQ, LPF_ORDER);
    if(ma_lpf_node_init(m_maNodeGraph.get(), &m_maLpfConfig, nullptr, m_maNodeLpf.get()) != MA_SUCCESS) {
        ma_node_graph_uninit(m_maNodeGraph.get(), nullptr);
        ma_device_uninit(m_maDevice.get());
        throw std::runtime_error("Couldn't initialize sound node graph!");
    }

    ma_node_attach_output_bus(m_maNodeLpf.get(), 0, ma_node_graph_get_endpoint(m_maNodeGraph.get()), 0);
}

Sound::~Sound() {
    stop();
};

void Sound::deleteNodeGraph(ma_node_graph *ng) {
    ma_node_graph_uninit(ng, nullptr);
}

void Sound::deleteLpfNode(ma_lpf_node *node) {
    ma_lpf_node_uninit(node, nullptr);
}

void Sound::deleteDataNode(ma_data_source_node *node) {
    ma_data_source_node_uninit(node, nullptr);
}

void Sound::dataCallback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount) {

    auto *instance = static_cast<Sound*>(pDevice->pUserData);
    MA_ASSERT(pDevice->playback.channels == instance->CHANNEL_COUNT);

    // Calculate how many system clocks there are in a frame:
    // callbackFrequency = instance->SAMPLE_RATE / frameCount;
    // expectedSystemClocks = instance->m_expectedClockRate / callbackFrequency;
    // clocksPerFrame = expectedSystemClocks / frameCount;
    // After substitution:
    unsigned long clocksPerFrame = instance->m_expectedClockRate / instance->SAMPLE_RATE;

    float *floatOut = static_cast<float*>(pOutput);

    // Get required number of frames.
    for(ma_uint32 fr = 0; fr < frameCount; fr++) {

        for(unsigned long clk = 0; clk < clocksPerFrame; clk++) {
            instance->m_systemClockFunction();
        }

        ma_node_graph_read_pcm_frames(instance->m_maNodeGraph.get(), floatOut, 1, nullptr);
        floatOut += 2;
    }
}

void Sound::configureSound(SoundConfig cfg) {

    stop();

    m_systemClockFunction = std::move(cfg.systemClock);
    m_expectedClockRate = cfg.systemClockSpeed;

    for(auto & source : cfg.sampleSources) {

        m_maDataSources.emplace_back(new callbackDataSource_t, &callBackDataSourceUninit);
        if(callBackDataSourceInit(m_maDataSources.back().get(), std::move(source)) != MA_SUCCESS) {
            // Log. Todo.
        } else {

            ma_data_source_node_config nodeConfig = ma_data_source_node_config_init(m_maDataSources.back().get());
            m_maNodesDataSource.emplace_back(new ma_data_source_node, &deleteDataNode);

            if(ma_data_source_node_init(m_maNodeGraph.get(), &nodeConfig, nullptr, m_maNodesDataSource.back().get()) != MA_SUCCESS) {
                m_maDataSources.pop_back();
                // log, todo
            } else {

                ma_node_attach_output_bus(m_maNodesDataSource.back().get(), 0, m_maNodeLpf.get(), 0);
                //ma_node_attach_output_bus(m_maNodesDataSource.back().get(), 0, ma_node_graph_get_endpoint(m_maNodeGraph.get()), 0);
            }
        }
    }

    m_configured = true;
}

void Sound::unloadConfig() {

    stop();

    // First clean the nodes, then the data sources which were encapsulated by the nodes.
    m_maNodesDataSource.clear();
    m_maDataSources.clear();

    m_configured = false;
}

bool Sound::soundAvailable() const {
    return m_configured;
}

void Sound::start() {

    if(m_running)
        return;

    if(m_configured) {
        if(ma_device_start(m_maDevice.get()) != MA_SUCCESS)
            throw std::runtime_error("Couldn't start sound device!");
        else
            m_running = true;
    }
}

void Sound::stop() {

    if(!m_running) return;

    if(ma_device_stop(m_maDevice.get()) != MA_SUCCESS)
        throw std::runtime_error("Couldn't stop sound device!");
    else
        m_running = false;
}
