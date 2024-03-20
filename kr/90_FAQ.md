이 페이지에서는 Vulkan 응용 프로그램 개발 도중 마주치게 되는 흔한 문제들에 대한 해결법을 알려 드립니다.

## core 검증 레이어에서 access violation error가 발생해요

MSI Afterburner / RivaTuner Statistics Server가 실행되고 있진 않은지 확인하세요. Vulkan과의 호환성 문제가 있습니다.

## 검증 레이어에서 아무런 메시지가 보이지 않아요 / 검증 레이어를 사용할 수 없어요

프로그램 종료시에 터미널이 열려있게 해서 검증 레이어가 오류를 출력할 수 있도록 하세요. 비주얼 스튜디오에서는 F5 대신 Ctrl-F5로 실행하면 되고, 리눅스에서는 터미널 윈도우에서 프로그램을 실행하면 됩니다. 여전히 아무 메시지가 나오지 않으면 검증 레이어가 활성화 되었는지 확인하시고, Vulkan SDK가 제대로 설치되었는지를 [이 페이지](https://vulkan.lunarg.com/doc/view/1.2.135.0/windows/getting_started.html)의 "설치 확인" 안내에 따라 확인해 보세요. 또한 SDK의 버전이 1.1.106.0 이상이어야 `VK_LAYER_KHRONOS_validation` 레이어가 지원됩니다.

## SteamOverlayVulkanLayer64.dll에서 vkCreateSwapchainKHR 오류가 발생해요

Steam 클라이언트 베타에 호환성 문제가 있습니다. 해결 방법은 몇 가지가 있습니다:
    * Steam 베타 프로그램 탈되하기
    * `DISABLE_VK_LAYER_VALVE_steam_overlay_1` 환경 변수를 `1`로 설정하기
    * `HKEY_LOCAL_MACHINE\SOFTWARE\Khronos\Vulkan\ImplicitLayers` 아래 레지스트리에서 Steam 오버레이 Vulkan 레이어를 삭제하기

예시:

![](/images/steam_layers_env.png)

## VK_ERROR_INCOMPATIBLE_DRIVER가 나오며 vkCreateInstance가 실패해요

MacOS에서 최신 MoltenSDK를 사용 중이시라면 `vkCreateInstance`가 `VK_ERROR_INCOMPATIBLE_DRIVER` 오류를 반환할 수 있습니다. 이는 [Vulkan SDK 버전 1.3.216 이상](https://vulkan.lunarg.com/doc/sdk/1.3.216.0/mac/getting_started.html)에서는 MoltenSDK를 사용하려면 `VK_KHR_PORTABILITY_subset` 확장을 활성화해야 하기 때문인데, 현재는 적합성이 완전히 검토되지 않았기 때문입니다.

`VkInstanceCreateInfo`에 `VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR` 플래그를 추가해야 하고 `VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME`를 인스턴스 확장 리스트에 추가해야 합니다.

코드 예시:

```c++
...

std::vector<const char*> requiredExtensions;

for(uint32_t i = 0; i < glfwExtensionCount; i++) {
    requiredExtensions.emplace_back(glfwExtensions[i]);
}

requiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

createInfo.enabledExtensionCount = (uint32_t) requiredExtensions.size();
createInfo.ppEnabledExtensionNames = requiredExtensions.data();

if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
    throw std::runtime_error("failed to create instance!");
}
```
