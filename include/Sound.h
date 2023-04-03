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

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "ringbuffer.hpp"

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
    static const size_t SAMPLE_BUFFER_SIZE = 8192;

    /// Number of PCM frames processed per second (two samples for stereo in a single frame).
    static const int SAMPLE_RATE   = 44100;
    static const int CHANNEL_COUNT = 2;

    // Low Pass Filter parameters.
    static const int   LPF_CUTOFF_FREQ   = 20000;
    static const int   LPF_ORDER           = 8;
    //const float LPF_BIAS            = 0.9f;

    // ===========================================
    // Miniaudio data source definition
    // ===========================================
    struct callbackDataSource_t {
        ma_data_source_base base;
         ma_pcm_rb *sampleBuffer;
    };

    static ma_result callbackDataSourceRead(ma_data_source* pDataSource, void* pFramesOut, ma_uint64 frameCount, ma_uint64* pFramesRead)
    {
        /* Since there's no notion of an end, we don't ever want to return MA_AT_END here. But it is possible to return 0. */
        callbackDataSource_t *dataSource = static_cast<callbackDataSource_t*>(pDataSource);
        MA_ASSERT(dataSource != nullptr);
        ma_pcm_rb *pRB = dataSource->sampleBuffer;
        MA_ASSERT(pRB != nullptr);
        ma_result result;
        ma_uint64 totalFramesRead;

        /* We need to run this in a loop since the ring buffer itself may loop. */
        totalFramesRead = 0;
        while (totalFramesRead < frameCount) {
            void* pMappedBuffer;
            ma_uint32 mappedFrameCount;
            ma_uint64 framesToRead = frameCount - totalFramesRead;
            if (framesToRead > 0xFFFFFFFF) {
                framesToRead = 0xFFFFFFFF;
            }

            mappedFrameCount = (ma_uint32)framesToRead;
            result = ma_pcm_rb_acquire_read(pRB, &mappedFrameCount, &pMappedBuffer);
            if (result != MA_SUCCESS) {
                break;
            }

            if (mappedFrameCount == 0) {
                break;  /* <-- End of ring buffer. */
            }

            ma_copy_pcm_frames(ma_offset_pcm_frames_ptr(pFramesOut, totalFramesRead, pRB->format, pRB->channels), pMappedBuffer, mappedFrameCount, pRB->format, pRB->channels);

            result = ma_pcm_rb_commit_read(pRB, mappedFrameCount);
            if (result != MA_SUCCESS) {
                break;
            }

            totalFramesRead += mappedFrameCount;
        }

        *pFramesRead = totalFramesRead;
        return MA_SUCCESS;
    }

    static ma_result callbackDataSourceGetDataFormat(ma_data_source* pDataSource, ma_format* pFormat, ma_uint32* pChannels, ma_uint32* pSampleRate, ma_channel* pChannelMap, size_t channelMapCap)
    {
        callbackDataSource_t *dataSource = static_cast<callbackDataSource_t*>(pDataSource);
        MA_ASSERT(dataSource != nullptr);
        ma_pcm_rb *pRB = dataSource->sampleBuffer;
        MA_ASSERT(pRB != nullptr);

        MA_ASSERT(pRB != nullptr);

        if (pFormat != nullptr) {
            *pFormat = pRB->format;
        }

        if (pChannels != nullptr) {
            *pChannels = pRB->channels;
        }

        if (pSampleRate != nullptr) {
            *pSampleRate = pRB->sampleRate;
        }

        /* Just assume the default channel map. */
        if (pChannelMap != nullptr) {
            ma_channel_map_init_standard(ma_standard_channel_map_default, pChannelMap, channelMapCap, pRB->channels);
        }

        return MA_SUCCESS;
    }

    static ma_result callbackDataSourceSeek(ma_data_source* pDataSource, ma_uint64 frameIndex) { return MA_NOT_IMPLEMENTED; }
    static ma_result callbackDataSourceGetCursor(ma_data_source* pDataSource, ma_uint64* pCursor) { return MA_NOT_IMPLEMENTED; }
    static ma_result callbackDataSourceGetLength(ma_data_source* pDataSource, ma_uint64* pLength) { return MA_NOT_IMPLEMENTED; }

    constexpr static ma_data_source_vtable g_my_data_source_vtable {
            callbackDataSourceRead,
            callbackDataSourceSeek,
            callbackDataSourceGetDataFormat,
            callbackDataSourceGetCursor,
            callbackDataSourceGetLength
    };

    static ma_result callBackDataSourceInit(callbackDataSource_t* pMyDataSource, ma_pcm_rb *sampleBuffer)
    {
        ma_result result;
        ma_data_source_config baseConfig;

        baseConfig = ma_data_source_config_init();
        baseConfig.vtable = &g_my_data_source_vtable;

        result = ma_data_source_init(&baseConfig, &pMyDataSource->base);
        if (result != MA_SUCCESS) {
            return result;
        }

        pMyDataSource->sampleBuffer = sampleBuffer;

        return MA_SUCCESS;
    }

    static void callBackDataSourceUninit(callbackDataSource_t* pMyDataSource)
    {
        // ... do the uninitialization of your custom data source here ...

        // You must uninitialize the base data source.
        ma_data_source_uninit(&pMyDataSource->base);
    }

    // ===========================================
    // Miniaudio data
    // ===========================================
    ma_device_config m_maConfig;
    ma_lpf_node_config  m_maLpfConfig;
    ma_node_graph_config  m_maNodeGraphConfig;
    static void deleteCallbackDataSource(callbackDataSource_t *ds);
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
    // These are the sample sources.
    std::vector<std::unique_ptr<callbackDataSource_t, decltype(&deleteCallbackDataSource)>> m_maDataSources;
    // These are the nodes encapsulating the sample sources.
    std::vector<std::unique_ptr<ma_data_source_node, decltype(&deleteDataNode)>> m_maNodesDataSource;

    // ===========================================
    // Audio buffer
    // ===========================================
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
    void writeFrame(size_t outputIndex, SoundStereoFrame frame);
    [[nodiscard]] static int getSampleRate() ;
};

#endif //USE_SOUND_H
