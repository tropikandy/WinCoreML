/**
 * Model Registry Implementation
 */

#include "model_registry.h"
#include <algorithm>

ModelRegistry::ModelRegistry() {}

ModelRegistry::~ModelRegistry() {
    Clear();
}

CmwErrorCode ModelRegistry::RegisterModel(const ModelMetadata& metadata) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (metadata.model_id.empty()) {
        return CMW_ERROR_INVALID_ARGUMENT;
    }

    models_[metadata.model_id] = metadata;
    return CMW_SUCCESS;
}

CmwErrorCode ModelRegistry::UnregisterModel(const std::string& model_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = models_.find(model_id);
    if (it == models_.end()) {
        return CMW_ERROR_MODEL_NOT_FOUND;
    }

    models_.erase(it);
    return CMW_SUCCESS;
}

CmwErrorCode ModelRegistry::GetModel(
    const std::string& model_id,
    ModelMetadata& out_metadata
) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = models_.find(model_id);
    if (it == models_.end()) {
        return CMW_ERROR_MODEL_NOT_FOUND;
    }

    out_metadata = it->second;
    return CMW_SUCCESS;
}

bool ModelRegistry::HasModel(const std::string& model_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return models_.find(model_id) != models_.end();
}

std::vector<std::string> ModelRegistry::ListModels() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> model_ids;
    model_ids.reserve(models_.size());

    for (const auto& pair : models_) {
        model_ids.push_back(pair.first);
    }

    return model_ids;
}

std::vector<ModelMetadata> ModelRegistry::GetAllModels() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<ModelMetadata> all_models;
    all_models.reserve(models_.size());

    for (const auto& pair : models_) {
        all_models.push_back(pair.second);
    }

    return all_models;
}

void ModelRegistry::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    models_.clear();
}
