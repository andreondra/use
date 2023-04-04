/**
 * @file Sound.h
 * @author Ondrej Golasowski (golasowski.o@gmail.com)
 * @brief USE multi-platform sound handler.
 * @copyright Copyright (c) 2023 Ondrej Golasowski
 */
#ifndef USE_SOUND_H
#define USE_SOUND_H

#include <memory>
#include "miniaudio.h"
#include "Types.h"

/**
 * USE sound manager.
 *
 * The Sound class handles all sounds produced by the components, mixes and plays them.
 * Multi-platform audio provided by miniaudio.h.
 *
 * How to use this class:
 * 1) Construct using correct number of required output nodes.
 * 2) Start the sound device using start().
 * 3) Periodically call writeSamples according to sample rate (which can be found using getSampleRate).
 * 4) Enjoy the sound.
 * 5) Stop the device using stop().
 *
 * The class mixes the output of all the devices, passes them through LPF and outputs to the sound device.
 * Ring buffer is used to store samples in advance to battle crackling when there are not enough samples to play
 * when the dataCallback is called (missed deadline problem). If the buffer is not available at the moment of
 * writing, a cache is used instead, which is flushed to the buffer in the next write where the buffer is available.
 *
 * Note about terms used:
 * This class uses same terminology as miniaudio.h documentation - frame consists of samples, the count equals the number
 * of audio channels. This means stereo frame consists of a left speaker sample followed by the right speaker sample.
 * */
class Sound {

private:
    bool m_running = false;

    // ===========================================
    // Audio parameters
    // ===========================================
    /// Sample buffer size.
    static const size_t SAMPLE_BUFFER_SIZE               = 32768;
    /// Minimal distance between read and write buffer pointer.
    static const size_t SAMPLE_BUFFER_MIN_PTR_DISTANCE   = 2048;
    /// Maximal distance between read and write buffer pointer.
    static const size_t SAMPLE_BUFFER_MAX_PTR_DISTANCE   = SAMPLE_BUFFER_SIZE - SAMPLE_BUFFER_MIN_PTR_DISTANCE;
    /// Buffer pointer shift if the distance becomes too large (small)
    static const size_t BUFFER_PTR_CORRECTION            = 16384;

    /// Number of PCM frames processed per second (two samples for stereo in a single frame).
    static const int SAMPLE_RATE   = 44100;
    /// Audio channel count.
    static const int CHANNEL_COUNT = 2;

    // Low Pass Filter parameters.
    static const int   LPF_CUTOFF_FREQ   = 20000;
    static const int   LPF_ORDER           = 8;

    // ===========================================
    // Miniaudio data
    // ===========================================
    ma_device_config m_maConfig;
    ma_lpf_node_config  m_maLpfConfig;
    ma_node_graph_config  m_maNodeGraphConfig;
    // Custom deleters for unique_ptr encapsulated miniaudio objects.
    static void deleteMaDevice(ma_device *device);
    static void deleteNodeGraph(ma_node_graph *ng);
    static void deleteLpfNode(ma_lpf_node *node);
    static void deleteDataNode(ma_data_source_node *node);
    static void deletePcmRb(ma_pcm_rb *rb);
    // RAII encapsulation for miniaudio structs (they require custom deleter).
    // Warning: maintain the order of the members, so the destruction happens in the correct order.
    // Destruction order is guaranteed by C++ standard 12.6.2.
    std::unique_ptr<ma_device,     decltype(&deleteMaDevice) >  m_maDevice   {new ma_device, &deleteMaDevice};
    std::unique_ptr<ma_node_graph, decltype(&deleteNodeGraph)>  m_maNodeGraph{new ma_node_graph, &deleteNodeGraph};
    std::unique_ptr<ma_lpf_node,   decltype(&deleteLpfNode)  >  m_maNodeLpf  {new ma_lpf_node, &deleteLpfNode};

    // The sound is processed in a following way:
    //  +-------------------+
    // |                   |
    // |  Frame  source 1  |---+
    // |                   |   |
    // +-------------------|   |        +-------------------+
    //                         |        |                   |
    //         ...             +------->|  Low Pass Filter  |
    //                         |        |                   |
    // +-------------------|   |        +-------------------+
    // |                   |   |
    // |  Frame  source x  |---+
    // |                   |
    // +-------------------+
    /// These are the nodes encapsulating the sample sources.
    std::vector<std::unique_ptr<ma_data_source_node, decltype(&deleteDataNode)>> m_maNodesDataSource;

    /// Main audio callback. Audio devices asks node graph for more samples in this function.
    static void dataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

    // ===========================================
    // Audio buffering
    // ===========================================
    /// Frame buffers. Serve also as data source nodes.
    std::vector<std::unique_ptr<ma_pcm_rb, decltype(&deletePcmRb)>> m_sampleBuffers;
    /// Temporary storage for audio frames if the shared ring buffer is not available at the moment of writing.
    std::vector<std::vector<float>> m_sampleCaches;

public:

    /**
     * Create sound device and prepare node graph with specified number of sources.
     *
     * @param outputCount Required number of input nodes. Equals the total amount of Components' audio outputs in the System.
     * */
    explicit Sound(size_t outputCount);
    ~Sound();

    /**
     * Start the sound device (which executes dataCallback periodically to asks for more audio frames).
     * */
    void start();

    /**
     * Stop the sound device.
     * */
    void stop();

    /**
     * Write audio frames to their respective buffers (or caches, if the buffer is currently used by the audio callback).
     *
     * @param sources Sound sources to take frames from.
     * */
    void writeFrames(const SoundSampleSources & sources);

    /**
     * Get sample rate of the audio device.
     *
     * @note Used mainly to generate appropriate number of audio frames per second (which are then passsed to writeFrames).
     * @return Sample rate.
     * */
    [[nodiscard]] static constexpr int getSampleRate() {
        return SAMPLE_RATE;
    };
};

#endif //USE_SOUND_H
