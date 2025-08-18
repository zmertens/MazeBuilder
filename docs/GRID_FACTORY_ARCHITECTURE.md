# Modern Grid Factory Architecture

## Overview

The grid factory has been redesigned to use a modern C++ registration pattern that provides flexibility, type safety, and extensibility while maintaining backward compatibility.

## Key Features

### 1. Registration-Based Creation
- Use `std::function` for type-erased function objects
- Register custom grid creators with unique string keys
- Thread-safe registration and creation operations

### 2. Modern C++ Features
- Uses C++20 features where appropriate
- RAII and smart pointer management
- Move semantics for optimal performance
- Exception safety

### 3. Backward Compatibility
- Original `create(const configurator&)` method still works
- Existing code continues to function without modification
- Default creators are automatically registered

## Basic Usage

### Traditional Usage (Still Supported)
```cpp
grid_factory factory;
configurator config;
config.rows(10).columns(10).levels(1).seed(12345);

auto grid = factory.create(config);  // Works as before
```

### New Registration Pattern
```cpp
grid_factory factory;

// Register a custom creator
factory.register_creator("my_grid", [](const configurator& config) {
    return std::make_unique<grid>(config.rows() * 2, config.columns() * 2, config.levels());
});

// Use the registered creator
configurator config;
config.rows(10).columns(10).levels(1);
auto result = factory.create("my_grid", config);
```

## Advanced Patterns

### 1. Conditional Registration
```cpp
grid_factory factory;

bool use_optimized = check_system_capabilities();
if (use_optimized) {
    factory.register_creator("adaptive_grid", [](const configurator& config) {
        return std::make_unique<optimized_grid>(config.rows(), config.columns(), config.levels());
    });
} else {
    factory.register_creator("adaptive_grid", [](const configurator& config) {
        return std::make_unique<grid>(config.rows(), config.columns(), config.levels());
    });
}
```

### 2. Validation and Error Handling
```cpp
factory.register_creator("safe_grid", [](const configurator& config) -> std::unique_ptr<grid_interface> {
    if (config.rows() == 0 || config.columns() == 0) {
        return nullptr;  // Invalid configuration
    }
    if (config.rows() * config.columns() > MAX_SAFE_SIZE) {
        return nullptr;  // Too large
    }
    return std::make_unique<grid>(config.rows(), config.columns(), config.levels());
});
```

### 3. Captured State
```cpp
int scale_factor = 3;
factory.register_creator("scaled_grid", [scale_factor](const configurator& config) {
    return std::make_unique<grid>(
        config.rows() * scale_factor, 
        config.columns() * scale_factor, 
        config.levels()
    );
});
```

## Factory Management

### Registration
```cpp
grid_factory factory;

// Register a creator
bool success = factory.register_creator("key", creator_function);

// Check if registered
bool exists = factory.is_registered("key");

// Get all registered keys
auto keys = factory.get_registered_keys();
```

### Unregistration and Cleanup
```cpp
// Unregister a creator
bool removed = factory.unregister_creator("key");

// Clear all creators (re-registers defaults)
factory.clear();
```

## Default Registered Creators

The factory automatically registers these creators:

| Key | Description |
|-----|-------------|
| `"grid"` | Basic grid implementation |
| `"distance_grid"` | Grid with distance calculation capabilities |
| `"colored_grid"` | Grid with color information for visualization |
| `"image_grid"` | Automatically chooses colored_grid or grid based on config |
| `"text_grid"` | Automatically chooses distance_grid or grid based on config |

## Thread Safety

- All registration operations are thread-safe
- Multiple threads can safely register different creators
- Grid creation is thread-safe when using different factory instances
- Shared factory instances are safe for concurrent read operations

## Helper Functions

Use the provided helper functions for common patterns:

```cpp
#include <MazeBuilder/grid_factory_helpers.h>

grid_factory factory;

// Register common patterns
int registered = factory_helpers::register_all_common_patterns(factory, "common");

// Register specific patterns
factory_helpers::register_scaled_grid(factory, "double_size", 2);
factory_helpers::register_clamped_grid(factory, "small", 1, 50, 1, 50);
factory_helpers::register_smart_grid(factory, "auto");
```

## Migration Guide

### For Existing Code
No changes required - existing code continues to work unchanged.

### For New Code
Consider using the registration pattern for:
- Custom grid types
- Configuration-dependent grid selection
- Performance-optimized variants
- Validation and error handling
- Testing with mock implementations

### Best Practices
1. Use descriptive keys for registration
2. Always check return values from registration operations
3. Prefer lambda functions for simple creators
4. Use helper functions for common patterns
5. Handle nullptr returns from creation operations
6. Consider thread safety in multi-threaded applications

## Architecture Benefits

1. **Extensibility**: Easy to add new grid types without modifying factory code
2. **Testability**: Can register mock implementations for testing
3. **Flexibility**: Runtime configuration of available grid types
4. **Performance**: Function objects are optimized by the compiler
5. **Type Safety**: Compile-time checking of creator function signatures
6. **Memory Safety**: RAII and smart pointers prevent memory leaks
7. **Thread Safety**: Concurrent access protection
8. **Backward Compatibility**: Existing code continues to work
