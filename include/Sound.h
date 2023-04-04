/**
 * @file Sound.h
 * @author Ondrej Golasowski (golasowski.o@gmail.com)
 * @brief USE multi-platform sound handler.
 * @copyright Copyright (c) 2023 Ondrej Golasowski
 */
#ifndef USE_SOUND_H
#define USE_SOUND_H

#include <functional>
#include <memory>
#include <cassert>

#include "miniaudio.h"
#include "Types.h"

/**
 * USE sound manager.
 *
 * The Sound class handles all sounds produced by the components, mixes and plays them.
 * Multi-platform audio provided by miniaudio.h.
 * */
class Sound {

private:
    bool m_running = false;

    // ===========================================
    // Audio parameters
    // ===========================================

    // Sample buffer size.
    static const size_t SAMPLE_BUFFER_SIZE               = 16384;
    static const size_t SAMPLE_BUFFER_MIN_PTR_DISTANCE   = 2024;
    static const size_t SAMPLE_BUFFER_MAX_PTR_DISTANCE   = SAMPLE_BUFFER_SIZE - SAMPLE_BUFFER_MIN_PTR_DISTANCE;
    static const size_t BUFFER_PTR_CORRECTION            = (SAMPLE_BUFFER_SIZE / 2);

    /// Number of PCM frames processed per second (two samples for stereo in a single frame).
    static const int SAMPLE_RATE   = 44100;
    static const int CHANNEL_COUNT = 2;

    // Low Pass Filter parameters.
    static const int   LPF_CUTOFF_FREQ   = 20000;
    static const int   LPF_ORDER           = 8;
    //const float LPF_BIAS            = 0.9f;

    // ===========================================
    // Miniaudio data
    // ===========================================
    ma_device_config m_maConfig;
    ma_lpf_node_config  m_maLpfConfig;
    ma_node_graph_config  m_maNodeGraphConfig;
    static void deleteMaDevice(ma_device *device);
    static void deleteNodeGraph(ma_node_graph *ng);
    static void deleteLpfNode(ma_lpf_node *node);
    static void deleteDataNode(ma_data_source_node *node);
    static void deletePcmRb(ma_pcm_rb *rb);
    // RAII encapsulation for miniaudio structs (they require custom deleter).
    // Warning: maintain the order of the members, so the destruction happens in the correct order too (see C++ standard 12.6.2).
    std::unique_ptr<ma_device, decltype(&deleteMaDevice)> m_maDevice{new ma_device, &deleteMaDevice};
    std::unique_ptr<ma_node_graph, decltype(&deleteNodeGraph)> m_maNodeGraph{new ma_node_graph, &deleteNodeGraph};
    std::unique_ptr<ma_lpf_node, decltype(&deleteLpfNode)> m_maNodeLpf{new ma_lpf_node, &deleteLpfNode};

    // The sound is processed in a following way:
    //  +-------------------+
    // |                   |
    // |  Sample source 1  |---+
    // |                   |   |
    // +-------------------|   |        +-------------------+
    //                         |        |                   |
    //         ...             +------->|  Low Pass Filter  |
    //                         |        |                   |
    // +-------------------|   |        +-------------------+
    // |                   |   |
    // |  Sample source x  |---+
    // |                   |
    // +-------------------+
    // These are the nodes encapsulating the sample sources.
    std::vector<std::unique_ptr<ma_data_source_node, decltype(&deleteDataNode)>> m_maNodesDataSource;

    // ===========================================
    // Audio buffer
    // ===========================================
    // Frame buffers. Serve also as data source nodes.
    std::vector<std::unique_ptr<ma_pcm_rb, decltype(&deletePcmRb)>> m_sampleBuffers;

    static void dataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

public:
    /**
     * Create sound device.
     * */
    explicit Sound(size_t outputCount);
    ~Sound();

    void start();
    void stop();
    void writeFrames(const SoundSampleSources & sources);
    [[nodiscard]] static int getSampleRate() ;
};

#endif //USE_SOUND_H
