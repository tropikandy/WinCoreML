/**
 * Unity C# Example for Universal ML Runtime
 *
 * This demonstrates how to use the UML Runtime from Unity via P/Invoke.
 * Place this script in your Unity Assets folder.
 */

using System;
using System.Runtime.InteropServices;
using UnityEngine;

public class UMLRuntimeWrapper : MonoBehaviour
{
    // ========================================================================
    // Native Imports
    // ========================================================================

    const string DLL_NAME = "umlruntime";

    // Error codes
    public enum ErrorCode
    {
        Success = 0,
        InvalidArgument = 1000,
        ModelNotFound = 2000,
        ServiceUnavailable = 4003,
    }

    // Data types
    public enum DataType
    {
        Float32 = 0,
        Float16 = 1,
        Int32 = 2,
        Int64 = 3,
        Int8 = 4,
        UInt8 = 5,
        Bool = 6,
    }

    // Compute units
    public enum ComputeUnits
    {
        CpuOnly = 0,
        CpuAndGpu = 1,
        All = 2,
        CpuAndNpu = 3,
    }

    // Opaque handles
    public struct RuntimeHandle { public IntPtr handle; }
    public struct ModelHandle { public IntPtr handle; }
    public struct TensorHandle { public IntPtr handle; }

    // Configuration structures
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct RuntimeConfig
    {
        public string pipeName;
        public int timeoutMs;
        public int enableLogging;
        public string logFilePath;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct InferenceConfig
    {
        public ComputeUnits computeUnits;
        public int enableProfiling;
        public int timeoutMs;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct TensorInfo
    {
        public string name;
        public DataType dtype;
        public int rank;
        public IntPtr shape; // int64_t*
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct BenchmarkResult
    {
        public string providerName;
        public float meanLatencyMs;
        public float speedupVsCpu;
    }

    // Native functions
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr UMLRTGetVersion();

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr UMLRTGetErrorString(ErrorCode code);

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern ErrorCode UMLRTCreateRuntime(
        ref RuntimeConfig config,
        out RuntimeHandle handle
    );

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern ErrorCode UMLRTDestroyRuntime(RuntimeHandle handle);

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern ErrorCode UMLRTRegisterModel(
        RuntimeHandle handle,
        [MarshalAs(UnmanagedType.LPStr)] string modelPath,
        out ModelHandle model
    );

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern ErrorCode UMLRTUnregisterModel(
        RuntimeHandle handle,
        ModelHandle model
    );

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern ErrorCode UMLRTGetInputCount(
        ModelHandle model,
        out int count
    );

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern ErrorCode UMLRTGetOutputCount(
        ModelHandle model,
        out int count
    );

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern ErrorCode UMLRTCreateTensor(
        [MarshalAs(UnmanagedType.LPStr)] string name,
        DataType dtype,
        int rank,
        long[] shape,
        IntPtr data,
        ulong dataSize,
        out TensorHandle tensor
    );

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern ErrorCode UMLRTDestroyTensor(TensorHandle tensor);

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern ErrorCode UMLRTRunInference(
        RuntimeHandle handle,
        ModelHandle model,
        [In] TensorHandle[] inputs,
        int inputCount,
        [Out] TensorHandle[] outputs,
        int outputCount,
        ref InferenceConfig config
    );

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern ErrorCode UMLRTCopyTensorData(
        TensorHandle tensor,
        IntPtr buffer,
        ulong bufferSize
    );

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern ErrorCode UMLRTGetBenchmarkResults(
        ModelHandle model,
        [Out] BenchmarkResult[] results,
        int maxResults,
        out int count
    );

    // ========================================================================
    // Unity Example Usage
    // ========================================================================

    private RuntimeHandle runtime;
    private ModelHandle model;

    void Start()
    {
        InitializeRuntime();
        LoadModel("Assets/Models/classifier.onnx");
    }

    void InitializeRuntime()
    {
        RuntimeConfig config = new RuntimeConfig
        {
            pipeName = null, // Use default
            timeoutMs = 30000,
            enableLogging = 1,
            logFilePath = null
        };

        ErrorCode result = UMLRTCreateRuntime(ref config, out runtime);
        if (result != ErrorCode.Success)
        {
            Debug.LogError($"Failed to create runtime: {result}");
            return;
        }

        string version = Marshal.PtrToStringAnsi(UMLRTGetVersion());
        Debug.Log($"UML Runtime initialized. Version: {version}");
    }

    void LoadModel(string modelPath)
    {
        Debug.Log($"Registering model: {modelPath}");

        ErrorCode result = UMLRTRegisterModel(runtime, modelPath, out model);
        if (result != ErrorCode.Success)
        {
            Debug.LogError($"Failed to register model: {result}");
            return;
        }

        Debug.Log("Model registered successfully!");

        // Get input/output counts
        UMLRTGetInputCount(model, out int inputCount);
        UMLRTGetOutputCount(model, out int outputCount);
        Debug.Log($"Model has {inputCount} inputs and {outputCount} outputs");

        // Get benchmark results
        BenchmarkResult[] benchmarks = new BenchmarkResult[5];
        UMLRTGetBenchmarkResults(model, benchmarks, 5, out int benchmarkCount);

        Debug.Log("Benchmark Results:");
        for (int i = 0; i < benchmarkCount; i++)
        {
            Debug.Log($"  {benchmarks[i].providerName}: " +
                     $"{benchmarks[i].meanLatencyMs:F2}ms " +
                     $"({benchmarks[i].speedupVsCpu:F2}x speedup)");
        }
    }

    void RunInference(float[] inputData)
    {
        // Create input tensor
        // Example: 1x3x224x224 image
        long[] shape = new long[] { 1, 3, 224, 224 };

        // Pin array and get pointer
        GCHandle handle = GCHandle.Alloc(inputData, GCHandleType.Pinned);
        IntPtr dataPtr = handle.AddrOfPinnedObject();

        TensorHandle inputTensor;
        ErrorCode result = UMLRTCreateTensor(
            "input",
            DataType.Float32,
            4,
            shape,
            dataPtr,
            (ulong)(inputData.Length * sizeof(float)),
            out inputTensor
        );

        handle.Free();

        if (result != ErrorCode.Success)
        {
            Debug.LogError($"Failed to create tensor: {result}");
            return;
        }

        // Prepare for inference
        TensorHandle[] inputs = new TensorHandle[] { inputTensor };
        TensorHandle[] outputs = new TensorHandle[1];

        InferenceConfig config = new InferenceConfig
        {
            computeUnits = ComputeUnits.All,
            enableProfiling = 0,
            timeoutMs = 5000
        };

        // Run inference
        result = UMLRTRunInference(
            runtime,
            model,
            inputs,
            1,
            outputs,
            1,
            ref config
        );

        if (result != ErrorCode.Success)
        {
            Debug.LogError($"Inference failed: {result}");
            UMLRTDestroyTensor(inputTensor);
            return;
        }

        Debug.Log("Inference successful!");

        // Copy output data
        // Assume output is 1x1000 (classification)
        float[] outputData = new float[1000];
        GCHandle outHandle = GCHandle.Alloc(outputData, GCHandleType.Pinned);

        UMLRTCopyTensorData(
            outputs[0],
            outHandle.AddrOfPinnedObject(),
            (ulong)(outputData.Length * sizeof(float))
        );

        outHandle.Free();

        // Find top prediction
        int maxIdx = 0;
        float maxVal = outputData[0];
        for (int i = 1; i < outputData.Length; i++)
        {
            if (outputData[i] > maxVal)
            {
                maxVal = outputData[i];
                maxIdx = i;
            }
        }

        Debug.Log($"Top prediction: Class {maxIdx} with confidence {maxVal:F4}");

        // Cleanup
        UMLRTDestroyTensor(inputTensor);
        UMLRTDestroyTensor(outputs[0]);
    }

    void OnDestroy()
    {
        if (model.handle != IntPtr.Zero)
        {
            UMLRTUnregisterModel(runtime, model);
        }

        if (runtime.handle != IntPtr.Zero)
        {
            UMLRTDestroyRuntime(runtime);
        }
    }

    // Example: Run inference when space is pressed
    void Update()
    {
        if (Input.GetKeyDown(KeyCode.Space))
        {
            // Generate random input for demo
            float[] randomInput = new float[1 * 3 * 224 * 224];
            for (int i = 0; i < randomInput.Length; i++)
            {
                randomInput[i] = UnityEngine.Random.Range(0f, 1f);
            }

            RunInference(randomInput);
        }
    }
}
