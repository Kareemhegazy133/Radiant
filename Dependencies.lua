-- Shared dependency include paths. Anchored to the workspace root so every
-- project script (which resolves relative paths against its own directory)
-- can use them unchanged.
IncludeDir = {}
IncludeDir["spdlog"]    = "%{wks.location}/Radiant/Vendor/spdlog/include"
IncludeDir["glm"]       = "%{wks.location}/Radiant/Vendor/glm"
IncludeDir["glfw"]      = "%{wks.location}/Radiant/Vendor/glfw/include"
IncludeDir["glad"]      = "%{wks.location}/Radiant/Vendor/glad/include"
IncludeDir["stb_image"] = "%{wks.location}/Radiant/Vendor/stb_image"
IncludeDir["entt"]      = "%{wks.location}/Radiant/Vendor/entt/include"
IncludeDir["box2d"]     = "%{wks.location}/Radiant/Vendor/box2d/include"
IncludeDir["yaml_cpp"]  = "%{wks.location}/Radiant/Vendor/yaml-cpp/include"
IncludeDir["ImGui"]     = "%{wks.location}/Radiant/Vendor/imgui"
