#include "Enginepch.h"

#include "UIElement.h"

namespace Engine {

    void UIElement::OnUpdate(Timestep ts)
    {
        if (GetComponent<MetadataComponent>().OnUpdate)
        {
            GetComponent<MetadataComponent>().OnUpdate(ts);
        }
    }
}