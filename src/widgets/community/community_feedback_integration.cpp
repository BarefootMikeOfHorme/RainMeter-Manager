#include "framework/widget_framework.h"
#include "community_feedback.h"
#include "ui_framework.h"
#include <Windows.h>
#include <CommCtrl.h>
#include <iostream>
#include <thread>

// Implementation of community feedback integration methods in WidgetManager

bool WidgetManager::createWidgetFromRecommendation(const SourceRecommendation& recommendation) {
    try {
        // Validate the recommendation first
        if (!recommendation.isValidated || recommendation.url.empty()) {
            std::cerr << "Cannot create widget from unvalidated or invalid recommendation" << std::endl;
            return false;
        }

        // Determine the appropriate widget type based on the recommendation
        WidgetType widgetType = WidgetUtils::detectStreamType(recommendation.url);
        std::string widgetId = "community_" + std::to_string(std::hash<std::string>{}(recommendation.url));

        std::shared_ptr<BaseWidget> widget = nullptr;

        // Create appropriate widget based on data type
        switch (recommendation.dataType) {
            case SourceRecommendation::DataType::VIDEO_STREAM: {
                StreamConfig config = WidgetUtils::createVideoStreamConfig(recommendation.url);
                config.refreshIntervalMs = recommendation.refreshRate;
                widget = createStreamingWidget(widgetId, WidgetType::VIDEO_STREAM, config);
                break;
            }
            case SourceRecommendation::DataType::AUDIO_STREAM: {
                StreamConfig config = WidgetUtils::createAudioStreamConfig(recommendation.url);
                config.refreshIntervalMs = recommendation.refreshRate;
                widget = createStreamingWidget(widgetId, WidgetType::AUDIO_STREAM, config);
                break;
            }
            case SourceRecommendation::DataType::IMAGE_FEED: {
                StreamConfig config = WidgetUtils::createImageFeedConfig(recommendation.url, recommendation.refreshRate);
                widget = createStreamingWidget(widgetId, WidgetType::IMAGE_FEED, config);
                break;
            }
            case SourceRecommendation::DataType::WEB_EMBED: {
                EmbedConfig config;
                config.embedUrl = recommendation.url;
                config.width = 400;
                config.height = 300;
                config.allowScripts = true;
                config.responsive = true;
                widget = createEmbedWidget(widgetId, config);
                break;
            }
            default:
                std::cerr << "Unsupported data type for widget creation" << std::endl;
                return false;
        }

        if (widget && widget->initialize()) {
            // Set metadata and start the widget
            widget->start();
            
            // Log the successful creation from community recommendation
            std::cout << "Successfully created widget from community recommendation: " 
                      << recommendation.title << " (" << recommendation.url << ")" << std::endl;
            
            // Update the recommendation usage stats
            auto& feedManager = CommunityFeedManager::getInstance();
            feedManager.incrementRecommendationUsage(recommendation.id);
            
            return true;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error creating widget from recommendation: " << e.what() << std::endl;
    }
    
    return false;
}

void WidgetManager::showFeedbackDialog(const std::string& sourceUrl, FeedbackType type) {
    try {
        // Create and show the feedback submission dialog
        auto dialog = std::make_unique<FeedbackSubmissionDialog>();
        
        if (!sourceUrl.empty()) {
            dialog->setSourceUrl(sourceUrl);
        }
        dialog->setFeedbackType(type);
        
        // Show the dialog modally
        if (dialog->showModal()) {
            std::cout << "Feedback dialog completed successfully" << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error showing feedback dialog: " << e.what() << std::endl;
        
        // Fallback: Show a simple message box
        MessageBoxA(nullptr, 
                   "Unable to show feedback dialog. Please visit our community portal.",
                   "Feedback System", 
                   MB_OK | MB_ICONINFORMATION);
    }
}

std::shared_ptr<SourceRecommendationWidget> WidgetManager::createRecommendationWidget() {
    try {
        std::string widgetId = "source_recommendations_" + std::to_string(GetTickCount64());
        auto widget = std::make_shared<SourceRecommendationWidget>(widgetId);
        
        if (registerWidget(std::static_pointer_cast<BaseWidget>(widget))) {
            return widget;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error creating recommendation widget: " << e.what() << std::endl;
    }
    
    return nullptr;
}

void WidgetManager::updateFeedListFromCommunity() {
    try {
        auto& feedManager = CommunityFeedManager::getInstance();
        
        // Get validated and approved recommendations
        auto recommendations = feedManager.getValidatedRecommendations();
        
        // Filter high-quality recommendations
        std::vector<SourceRecommendation> highQualityFeeds;
        for (const auto& rec : recommendations) {
            if (rec.qualityScore >= 0.8 && rec.isValidated && rec.category == "feed") {
                highQualityFeeds.push_back(rec);
            }
        }
        
        // Update feed list asynchronously to avoid blocking UI
        std::thread updateThread([this, highQualityFeeds]() {
            try {
                int updatesApplied = 0;
                
                for (const auto& feed : highQualityFeeds) {
                    // Check if we already have a widget for this feed
                    bool feedExists = false;
                    {
                        std::lock_guard<std::mutex> lock(widgetsMutex);
                        for (const auto& [id, widget] : widgets) {
                            // Simple check - in practice you'd want more sophisticated deduplication
                            if (id.find("community_") == 0) {
                                feedExists = true;
                                break;
                            }
                        }
                    }
                    
                    if (!feedExists) {
                        if (createWidgetFromRecommendation(feed)) {
                            updatesApplied++;
                        }
                    }
                }
                
                std::cout << "Applied " << updatesApplied << " feed updates from community recommendations" << std::endl;
            }
            catch (const std::exception& e) {
                std::cerr << "Error in feed update thread: " << e.what() << std::endl;
            }
        });
        
        updateThread.detach();
    }
    catch (const std::exception& e) {
        std::cerr << "Error updating feed list from community: " << e.what() << std::endl;
    }
}

void WidgetManager::enableCommunityFeatures(bool enable) {
    try {
        auto& feedManager = CommunityFeedManager::getInstance();
        
        if (enable) {
            // Initialize community features
            feedManager.initialize();
            
            // Create a recommendation widget if none exists
            bool hasRecommendationWidget = false;
            {
                std::lock_guard<std::mutex> lock(widgetsMutex);
                for (const auto& [id, widget] : widgets) {
                    if (id.find("source_recommendations") == 0) {
                        hasRecommendationWidget = true;
                        break;
                    }
                }
            }
            
            if (!hasRecommendationWidget) {
                auto recWidget = createRecommendationWidget();
                if (recWidget) {
                    recWidget->initialize();
                    std::cout << "Community recommendation widget created and initialized" << std::endl;
                }
            }
            
            // Set up periodic feed updates
            std::thread periodicUpdateThread([this]() {
                while (true) {
                    std::this_thread::sleep_for(std::chrono::hours(24)); // Update daily
                    updateFeedListFromCommunity();
                }
            });
            periodicUpdateThread.detach();
            
            std::cout << "Community features enabled" << std::endl;
        }
        else {
            // Disable community features
            std::cout << "Community features disabled" << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error enabling/disabling community features: " << e.what() << std::endl;
    }
}

// Additional utility implementations for community integration

namespace WidgetUtils {
    
bool validateCommunitySource(const std::string& url) {
    // Enhanced validation for community-submitted sources
    if (!isValidStreamUrl(url) && !isValidEmbedUrl(url)) {
        return false;
    }
    
    // Check against known problematic domains
    std::string domain = extractDomainFromUrl(url);
    std::vector<std::string> blockedDomains = {
        "malicious-site.com",
        "spam-feeds.net",
        "unreliable-source.org"
    };
    
    for (const auto& blocked : blockedDomains) {
        if (domain.find(blocked) != std::string::npos) {
            return false;
        }
    }
    
    return true;
}

std::string generateCommunityFeedConfig(const SourceRecommendation& recommendation) {
    std::string config = "{\n";
    config += "  \"id\": \"" + recommendation.id + "\",\n";
    config += "  \"title\": \"" + recommendation.title + "\",\n";
    config += "  \"url\": \"" + recommendation.url + "\",\n";
    config += "  \"category\": \"" + recommendation.category + "\",\n";
    config += "  \"refreshRate\": " + std::to_string(recommendation.refreshRate) + ",\n";
    config += "  \"qualityScore\": " + std::to_string(recommendation.qualityScore) + ",\n";
    config += "  \"submittedBy\": \"" + recommendation.submittedBy + "\",\n";
    config += "  \"tags\": [";
    for (size_t i = 0; i < recommendation.tags.size(); ++i) {
        config += "\"" + recommendation.tags[i] + "\"";
        if (i < recommendation.tags.size() - 1) config += ", ";
    }
    config += "]\n";
    config += "}";
    return config;
}

void logCommunityActivity(const std::string& activity, const std::string& details) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::cout << "[Community] " << std::ctime(&time_t) 
              << " - " << activity << ": " << details << std::endl;
}

} // namespace WidgetUtils
