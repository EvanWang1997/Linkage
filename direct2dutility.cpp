

//=================================================================================== 
// Copyright (c) Microsoft Corporation.  All rights reserved.                         
//                                                                                    
// THIS CODE AND INFORMATION IS PROVIDED 'AS IS' WITHOUT WARRANTY                     
// OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT                        
// LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND                           
// FITNESS FOR A PARTICULAR PURPOSE.                                                  
//=================================================================================== 
 
 
#include "StdAfx.h" 

#define  NTDDI_VERSION  0x06000000
#define WINVER 0x0600
#define _WIN32_WINNT 0x0600

//#include <wrl.h>
#include <string>
#include "Wincodecsdk.h"
#include "Direct2DUtility.h" 
 
using namespace Hilo::Direct2DHelpers; 

using namespace Microsoft::WRL;

Direct2DUtility::Direct2DUtility() 
{ 
} 
 
Direct2DUtility::~Direct2DUtility() 
{ 
} 
 
// 
// Creates a Direct2D bitmap from the specified 
// file name. 
// 
HRESULT Direct2DUtility::LoadBitmapFromFile( 
    ID2D1RenderTarget *renderTarget, 
    const wchar_t *uri, 
    unsigned int destinationWidth,  
    unsigned int destinationHeight, 
    ID2D1Bitmap ** bitmap) 
{ 
    HRESULT hr = S_OK; 
 
    ComPtr<IWICBitmapDecoder> decoder; 
    ComPtr<IWICBitmapFrameDecode> bitmapSource; 
	ComPtr<IWICStream> stream; 
    ComPtr<IWICFormatConverter> converter; 
    ComPtr<IWICBitmapScaler> scaler; 
    ComPtr<IWICImagingFactory> wicFactory; 
 
    hr = GetWICFactory(&wicFactory); 
    if (SUCCEEDED(hr)) 
    { 
        hr = wicFactory->CreateDecoderFromFilename( 
            uri, 
            nullptr, 
            GENERIC_READ, 
            WICDecodeMetadataCacheOnLoad, 
            &decoder); 
    } 
 
    if (SUCCEEDED(hr)) 
    { 
        // Create the initial frame. 
        hr = decoder->GetFrame(0, &bitmapSource); 
    } 
 
    if (SUCCEEDED(hr)) 
    { 
        // Convert the image format to 32bppPBGRA 
        // (DXGI_FORMAT_B8G8R8A8_UNORM + D2D1_ALPHA_MODE_PREMULTIPLIED). 
        hr = wicFactory->CreateFormatConverter(&converter); 
    } 
 
    if (SUCCEEDED(hr)) 
    { 
        // If a new width or height was specified, create an 
        // IWICBitmapScaler and use it to resize the image. 
        if (destinationWidth != 0 || destinationHeight != 0) 
        { 
            unsigned int originalWidth, originalHeight; 
            hr = bitmapSource->GetSize(&originalWidth, &originalHeight); 
            if (SUCCEEDED(hr)) 
            { 
                if (destinationWidth == 0) 
                { 
                    float scalar = static_cast<float>(destinationHeight) / static_cast<float>(originalHeight); 
                    destinationWidth = static_cast<unsigned int>(scalar * static_cast<float>(originalWidth)); 
                } 
                else if (destinationHeight == 0) 
                { 
                    float scalar = static_cast<float>(destinationWidth) / static_cast<float>(originalWidth); 
                    destinationHeight = static_cast<unsigned int>(scalar * static_cast<float>(originalHeight)); 
                } 
 
                hr = wicFactory->CreateBitmapScaler(&scaler); 
                if (SUCCEEDED(hr)) 
                { 
                    hr = scaler->Initialize( 
                        bitmapSource, 
                        destinationWidth, 
                        destinationHeight, 
                        WICBitmapInterpolationModeCubic); 
                } 
                if (SUCCEEDED(hr)) 
                { 
                    hr = converter->Initialize( 
                        scaler, 
                        GUID_WICPixelFormat32bppPBGRA, 
                        WICBitmapDitherTypeNone, 
                        nullptr, 
                        0.f, 
                        WICBitmapPaletteTypeMedianCut); 
                } 
            } 
        } 
        else // Don't scale the image. 
        { 
            hr = converter->Initialize( 
                bitmapSource, 
                GUID_WICPixelFormat32bppPBGRA, 
                WICBitmapDitherTypeNone, 
                nullptr, 
                0.f, 
                WICBitmapPaletteTypeMedianCut); 
        } 
    } 
 
    if (SUCCEEDED(hr)) 
    { 
        // Create a Direct2D bitmap from the WIC bitmap. 
        hr = renderTarget->CreateBitmapFromWicBitmap( 
            converter, 
            nullptr, 
            bitmap); 
    } 
 
    return hr; 
} 
 

 
// 
// Get a WIC Imaging factory 
// 
HRESULT Direct2DUtility::GetWICFactory(IWICImagingFactory** factory) 
{ 
    static IWICImagingFactory* m_pWICFactory; 
    HRESULT hr = S_OK; 
 
    if (nullptr == m_pWICFactory) 
    { 
        hr = CoCreateInstance( 
            CLSID_WICImagingFactory, 
            nullptr, 
            CLSCTX_INPROC_SERVER, 
            IID_PPV_ARGS(&m_pWICFactory)); 
    } 
 
    if (SUCCEEDED(hr)) 
    { 
        hr = AssignToOutputPointer(factory, m_pWICFactory); 
    } 
 
    return hr; 
} 
 
// 
// Get a DirectWrite factory 
// 
HRESULT Direct2DUtility::GetDWriteFactory(IDWriteFactory** factory ) 
{ 
    static IDWriteFactory* m_pDWriteFactory; 
    HRESULT hr = S_OK; 
 
    if (nullptr == m_pDWriteFactory) 
    { 
        hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(m_pDWriteFactory), reinterpret_cast<IUnknown**>(&m_pDWriteFactory)); 
    } 
 
    if (SUCCEEDED(hr)) 
    { 
        hr = AssignToOutputPointer(factory, m_pDWriteFactory); 
    } 
 
    return hr; 
}

