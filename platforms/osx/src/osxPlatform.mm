#include "osxPlatform.h"
#include "gl/hardware.h"
#include "log.h"

#import <cstdarg>
#import <cstdio>

#define DEFAULT "fonts/NotoSans-Regular.ttf"
#define FONT_AR "fonts/NotoNaskh-Regular.ttf"
#define FONT_HE "fonts/NotoSansHebrew-Regular.ttf"
#define FONT_JA "fonts/DroidSansJapanese.ttf"
#define FALLBACK "fonts/DroidSansFallback.ttf"

namespace Tangram {

void logMsg(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void setCurrentThreadPriority(int priority) {
    // POSIX thread priority is between -20 (highest) and 19 (lowest),
    // NSThread priority is between 0.0 (lowest) and 1.0 (highest).
    double p = (20 - priority) / 40.0;
    [[NSThread currentThread] setThreadPriority:p];
}

void initGLExtensions() {
    Tangram::Hardware::supportsMapBuffer = true;
}

void OSXPlatform::requestRender() const {
    glfwPostEmptyEvent();
}

OSXPlatform::OSXPlatform() {
    NSURLSessionConfiguration* configuration = [NSURLSessionConfiguration defaultSessionConfiguration];
    NSString *cachePath = [NSTemporaryDirectory() stringByAppendingPathComponent:@"/tile_cache"];
    NSURLCache *tileCache = [[NSURLCache alloc] initWithMemoryCapacity: 4 * 1024 * 1024 diskCapacity: 30 * 1024 * 1024 diskPath: cachePath];
    configuration.URLCache = tileCache;
    configuration.requestCachePolicy = NSURLRequestUseProtocolCachePolicy;
    configuration.timeoutIntervalForRequest = 30;
    configuration.timeoutIntervalForResource = 60;

    m_urlSession = [NSURLSession sessionWithConfiguration: configuration];
}

OSXPlatform::~OSXPlatform() {
    [m_urlSession getTasksWithCompletionHandler:^(NSArray* dataTasks, NSArray* uploadTasks, NSArray* downloadTasks) {
        for(NSURLSessionTask* task in dataTasks) {
            [task cancel];
        }
    }];
}

std::vector<FontSourceHandle> OSXPlatform::systemFontFallbacksHandle() const {
    std::vector<FontSourceHandle> handles;

    handles.emplace_back(DEFAULT);
    handles.emplace_back(FONT_AR);
    handles.emplace_back(FONT_HE);
    handles.emplace_back(FONT_JA);
    handles.emplace_back(FALLBACK);

    return handles;
}

UrlRequestHandle OSXPlatform::startUrlRequest(Url _url, UrlCallback _callback) {

    void (^handler)(NSData*, NSURLResponse*, NSError*) = ^void (NSData* data, NSURLResponse* response, NSError* error) {

        // Create our response object. The '__block' specifier is to allow mutation in the data-copy block below.
        __block UrlResponse urlResponse;

        // Check for errors from NSURLSession, then check for HTTP errors.
        if (error != nil) {

            urlResponse.error = [error.localizedDescription UTF8String];

        } else if ([response isKindOfClass:[NSHTTPURLResponse class]]) {

            NSHTTPURLResponse* httpResponse = (NSHTTPURLResponse*)response;
            int statusCode = [httpResponse statusCode];
            if (statusCode >= 400) {
                urlResponse.error = [[NSHTTPURLResponse localizedStringForStatusCode: statusCode] UTF8String];
            }
        }

        // Copy the data from the NSURLResponse into our URLResponse.
        // First we allocate the total data size.
        urlResponse.content.resize([data length]);
        // NSData may be stored in several ranges, so the 'bytes' method may incur extra copy operations.
        // To avoid that we copy the data in ranges provided by the NSData.
        [data enumerateByteRangesUsingBlock:^(const void * _Nonnull bytes, NSRange byteRange, BOOL * _Nonnull stop) {
            memcpy(urlResponse.content.data() + byteRange.location, bytes, byteRange.length);
        }];

        // Run the callback from the requester.
        if (_callback) {
            _callback(urlResponse);
        }
    };

    NSURL* nsUrl = [NSURL URLWithString:[NSString stringWithUTF8String:_url.string().c_str()]];
    NSURLSessionDataTask* dataTask = [m_urlSession dataTaskWithURL:nsUrl completionHandler:handler];

    [dataTask resume];

    return [dataTask taskIdentifier];

}

void OSXPlatform::cancelUrlRequest(UrlRequestHandle handle) {

    [m_urlSession getTasksWithCompletionHandler:^(NSArray* dataTasks, NSArray* uploadTasks, NSArray* downloadTasks) {
        for (NSURLSessionTask* task in dataTasks) {
            if ([task taskIdentifier] == handle) {
                [task cancel];
                break;
            }
        }
    }];
}

} // namespace Tangram
