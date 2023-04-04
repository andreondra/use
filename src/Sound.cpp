#include <stdexcept>

// Include implementation part of the miniaudio here.
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "Sound.h"

Sound::Sound(size_t outputCount) {

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
        m_maDevice.reset();
        throw std::runtime_error("Couldn't initialize sound node graph!");
    }

    m_maLpfConfig = ma_lpf_node_config_init(CHANNEL_COUNT, SAMPLE_RATE, LPF_CUTOFF_FREQ, LPF_ORDER);
    if(ma_lpf_node_init(m_maNodeGraph.get(), &m_maLpfConfig, nullptr, m_maNodeLpf.get()) != MA_SUCCESS) {
        m_maNodeGraph.reset();
        m_maDevice.reset();
        throw std::runtime_error("Couldn't initialize sound node graph!");
    }

    ma_node_attach_output_bus(m_maNodeLpf.get(), 0, ma_node_graph_get_endpoint(m_maNodeGraph.get()), 0);

    for(size_t i = 0; i < outputCount; i++) {

        m_sampleBuffers.emplace_back(new ma_pcm_rb, &deletePcmRb);
        if(ma_pcm_rb_init(ma_format_f32, CHANNEL_COUNT, SAMPLE_BUFFER_SIZE, nullptr, nullptr, m_sampleBuffers.back().get()) != MA_SUCCESS) {
            m_sampleBuffers.clear();
            throw std::runtime_error("Couldn't initialize output buffer.");
        } else {

            ma_data_source_node_config nodeConfig = ma_data_source_node_config_init(m_sampleBuffers.back().get());
            m_maNodesDataSource.emplace_back(new ma_data_source_node, &deleteDataNode);

            if(ma_data_source_node_init(m_maNodeGraph.get(), &nodeConfig, nullptr, m_maNodesDataSource.back().get()) != MA_SUCCESS) {
                m_sampleBuffers.clear();
                m_maNodesDataSource.clear();
                throw std::runtime_error("Couldn't initialize data source node.");
            } else {
                //ma_node_attach_output_bus(m_maNodesDataSource.back().get(), 0, m_maNodeLpf.get(), 0);
                ma_node_attach_output_bus(m_maNodesDataSource.back().get(), 0, ma_node_graph_get_endpoint(m_maNodeGraph.get()), 0);
            }
        }
    }
}

Sound::~Sound() {
    stop();
};

void Sound::deleteMaDevice(ma_device *device) {
    ma_device_uninit(device);
    delete device;
}

void Sound::deleteNodeGraph(ma_node_graph *ng) {
    ma_node_graph_uninit(ng, nullptr);
    delete ng;
}

void Sound::deleteLpfNode(ma_lpf_node *node) {
    ma_lpf_node_uninit(node, nullptr);
    delete node;
}

void Sound::deleteDataNode(ma_data_source_node *node) {
    ma_data_source_node_uninit(node, nullptr);
    delete node;
}

void Sound::deletePcmRb(ma_pcm_rb *rb) {
    ma_pcm_rb_uninit(rb);
    delete rb;
}

void Sound::dataCallback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount) {

    auto *instance = static_cast<Sound*>(pDevice->pUserData);
    assert(pDevice->playback.channels == Sound::CHANNEL_COUNT);

    // Calculate how many system clocks there are in a frame:
    // callbackFrequency = instance->SAMPLE_RATE / frameCount;
    // expectedSystemClocks = instance->m_expectedClockRate / callbackFrequency;
    // clocksPerFrame = expectedSystemClocks / frameCount;
    // After substitution:
    // unsigned long clocksPerFrame = instance->m_expectedClockRate / instance->SAMPLE_RATE;

    // Correcting buffers read pointer position.
    for(auto & buf : instance->m_sampleBuffers) {
        if(ma_pcm_rb_pointer_distance(buf.get()) > Sound::SAMPLE_BUFFER_MAX_PTR_DISTANCE) {
            ma_pcm_rb_seek_read(buf.get(), Sound::BUFFER_PTR_CORRECTION);
        }
    }

    ma_node_graph_read_pcm_frames(instance->m_maNodeGraph.get(), pOutput, frameCount, nullptr);
}

void Sound::start() {

    if(m_running)
        return;

    if(ma_device_start(m_maDevice.get()) != MA_SUCCESS)
        throw std::runtime_error("Couldn't start sound device!");
    else
        m_running = true;
}

void Sound::stop() {

    if(!m_running) return;

    if(ma_device_stop(m_maDevice.get()) != MA_SUCCESS)
        throw std::runtime_error("Couldn't stop sound device!");
    else
        m_running = false;
}

void Sound::writeFrames(const SoundSampleSources & sources) {

    if(sources.size() != m_sampleBuffers.size())
        throw std::invalid_argument("Sample sources and sample buffers size mismatch.");

    auto bufferIt = m_sampleBuffers.begin();
    for(auto & getSample : sources) {

        // Correcting write pointer position.
        if(ma_pcm_rb_pointer_distance(bufferIt->get()) < Sound::SAMPLE_BUFFER_MIN_PTR_DISTANCE) {
            ma_pcm_rb_seek_write(bufferIt->get(), Sound::BUFFER_PTR_CORRECTION);
        }

        void* mappedBuffer;
        ma_uint32 mappedFrameCount;
        float rawFrame[2] = {getSample().left, getSample().right};

        if(ma_pcm_rb_acquire_write(bufferIt->get(), &mappedFrameCount, &mappedBuffer) != MA_SUCCESS)
            continue;

        // TODO
        // if(mappedFrameCount < xxx) - move forward
        // Buffer full.
        if(mappedFrameCount == 0)
            continue;

        ma_copy_pcm_frames(mappedBuffer, &rawFrame, 1, bufferIt->get()->format, bufferIt->get()->channels);
        if(ma_pcm_rb_commit_write(bufferIt->get(), mappedFrameCount) != MA_SUCCESS)
            continue;

        bufferIt++;
    }
}

int Sound::getSampleRate() {
    return SAMPLE_RATE;
}