#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "vendor/Nexus.h"
#include "vendor/Mumble.h"
#include "vendor/imgui/imgui.h"

static AddonAPI_t* api;
static NexusLinkData_t* nexus;
static Mumble::Data* mumble;
static std::atomic<HWND> gameWindow{nullptr};
static std::atomic<unsigned> requests{0};
static std::atomic<bool> hold{false};
static constexpr const char* binds[] = {"INQUEST_HOLD", "INQUEST_TOGGLE", "INQUEST_DODGEJUMP", "INQUEST_SETTINGS"};
static bool active=false, prompt=false, show=false, requireMouse=false, muted=false;
static bool paused=false;
static float interval=1.0f, volume=8.0f;
static int dodgeKey='V', jumpKey=VK_SPACE;
static bool useGameBinds=true;
static POINT target{};
static ULONGLONG nextClick=0, nextHold=0, nextDodge=0, releaseAt=0, remaining=0;
static bool dodgePressed=false;
static std::string ini, soundPath;
static std::vector<unsigned char> sound;
static bool down(int key) { return (GetAsyncKeyState(key)&0x8000)!=0; }
static bool ready() {
    HWND w=gameWindow.load();
    return w && GetForegroundWindow()==w && nexus && nexus->IsGameplay && mumble &&
        mumble->UITick && mumble->Context.IsGameFocused && !mumble->Context.IsTextboxFocused &&
        !mumble->Context.IsMapOpen;
}
static bool busy() { return !ready() || mumble->Context.IsInCombat || down(VK_CONTROL) || down(VK_SHIFT) || down(VK_MENU) || down(VK_RBUTTON) || (nexus && nexus->IsCameraMoving); }
static void stop() { active=false; prompt=false; paused=false; }
static void save() {
    char b[64];
    auto put=[&](const char* k,int v){std::snprintf(b,sizeof(b),"%d",v);WritePrivateProfileStringA("Inquest",k,b,ini.c_str());};
    put("IntervalMs",(int)(interval*1000)); put("RequireLeftMouse",requireMouse); put("Muted",muted);
    put("VolumePercent",(int)volume); put("DodgeKey",dodgeKey); put("JumpKey",jumpKey); put("UseGameBinds",useGameBinds);
}
static void prepareSound() {
    PlaySoundA(nullptr,nullptr,0); sound.clear();
    FILE* f=std::fopen(soundPath.c_str(),"rb"); if(!f)return;
    std::fseek(f,0,SEEK_END); long n=std::ftell(f);std::rewind(f);
    if(n<44||n>2000000){std::fclose(f);return;}
    sound.resize(n); if(std::fread(sound.data(),1,n,f)!=(size_t)n){sound.clear();std::fclose(f);return;}std::fclose(f);
    // Only scale PCM 16-bit samples; retain every original WAV header/chunk.
    unsigned format=0,bits=0;
    for(size_t p=12;p+8<=sound.size();) {
        uint32_t len=0;std::memcpy(&len,&sound[p+4],4);if(len>sound.size()-p-8)break;
        if(!std::memcmp(&sound[p],"fmt ",4)&&len>=16){std::memcpy(&format,&sound[p+8],2);std::memcpy(&bits,&sound[p+22],2);}
        if(!std::memcmp(&sound[p],"data",4)&&format==1&&bits==16)
            for(size_t i=p+8;i+1<p+8+len;i+=2){int16_t v;std::memcpy(&v,&sound[i],2);v=(int16_t)(v*volume/100.f);std::memcpy(&sound[i],&v,2);}
        p+=8+len+(len&1);
    }
}
static void click(POINT p) {
    HWND w=gameWindow.load(); RECT r{};GetClientRect(w,&r);
    if(p.x<0||p.y<0||p.x>=r.right||p.y>=r.bottom)return;
    const LPARAM pos=MAKELPARAM(p.x,p.y);
    // Dispatch to GW2 only. The cursor is never moved or clicked in other apps.
    api->WndProc_SendToGameOnly(w,WM_LBUTTONDOWN,MK_LBUTTON,pos);
    api->WndProc_SendToGameOnly(w,WM_LBUTTONUP,0,pos);
    api->WndProc_SendToGameOnly(w,WM_LBUTTONDBLCLK,MK_LBUTTON,pos);
    api->WndProc_SendToGameOnly(w,WM_LBUTTONUP,0,pos);
    // Match the original module's follow-up click to clear GW2's double-click queue.
    api->WndProc_SendToGameOnly(w,WM_LBUTTONDOWN,MK_LBUTTON,pos);
    api->WndProc_SendToGameOnly(w,WM_LBUTTONUP,0,pos);
    if(!muted&&volume>0&&!sound.empty())PlaySoundA((LPCSTR)sound.data(),nullptr,SND_MEMORY|SND_ASYNC|SND_NODEFAULT);
}
static bool cursor(POINT& p){return GetCursorPos(&p)&&ScreenToClient(gameWindow.load(),&p);}
static void key(int vk,bool release) {
    const UINT scan=MapVirtualKeyA(vk,MAPVK_VK_TO_VSC);
    LPARAM flags=1|(scan<<16);if(release)flags|=(1u<<30)|(1u<<31);
    api->WndProc_SendToGameOnly(gameWindow.load(),release?WM_KEYUP:WM_KEYDOWN,vk,flags);
}
static bool usedGameBinds=false;
static void releaseDodge() {
    if(!dodgePressed)return;
    if(usedGameBinds){api->GameBinds_Release(GB_MoveDodge);api->GameBinds_Release(GB_MoveJump_SwimUp_FlyUp);}
    else {key(dodgeKey,true);key(jumpKey,true);}
    dodgePressed=false;
}
static void onBind(const char* id,bool release) {
    if(!std::strcmp(id,binds[0])) {hold=!release;return;}
    if(release)return;
    if(!std::strcmp(id,binds[1]))requests.fetch_or(1);
    else if(!std::strcmp(id,binds[2]))requests.fetch_or(2);
    else if(!std::strcmp(id,binds[3]))requests.fetch_or(4);
}
static UINT onWnd(HWND w,UINT msg,WPARAM,LPARAM) {
    gameWindow=w;
    if(msg==WM_KILLFOCUS || (msg==WM_ACTIVATEAPP && GetForegroundWindow()!=w)) {hold=false;requests.fetch_or(8);}
    return msg;
}
static void options() {
    ImGui::TextUnformatted("Inquest Chipset - Nexus port");
    ImGui::TextWrapped("Default shortcuts: hold comma for double clicks; [ for an interval prompt; Ctrl+Space for dodge-jump; Ctrl+Shift+I for this window. Change shortcuts in Nexus > Keybinds.");
    bool changed=false;
    changed|=ImGui::SliderFloat("Interval (seconds)",&interval,0.25f,300.f,"%.2f",ImGuiSliderFlags_Logarithmic);
    changed|=ImGui::Checkbox("Hold mode also requires left mouse",&requireMouse);
    changed|=ImGui::Checkbox("Mute click sound",&muted);
    if(ImGui::SliderFloat("Click sound volume",&volume,0,100,"%.0f%%")){prepareSound();changed=true;}
    changed|=ImGui::Checkbox("Use Nexus game bindings for dodge / jump",&useGameBinds);
    if(!useGameBinds){
        ImGui::TextWrapped("Fallback Windows virtual-key codes. Defaults: V = 86, Space = 32.");
        changed|=ImGui::InputInt("Dodge key",&dodgeKey);changed|=ImGui::InputInt("Jump key",&jumpKey);
        dodgeKey=std::clamp(dodgeKey,1,255);jumpKey=std::clamp(jumpKey,1,255);
    }
    if(changed)save();
    ImGui::TextUnformatted(active?(paused?"Double clicking: paused":"Double clicking: active"):"Double clicking: stopped");
    if(ImGui::Button("Stop all clicking")){stop();hold=false;}
    ImGui::TextWrapped("Clicking stops in combat or on leaving gameplay. Chat, map, camera movement, modifiers and loss of game focus pause input. Automation never resumes by itself after restarting GW2.");
}
static void render() {
    ULONGLONG now=GetTickCount64();
    if(dodgePressed&&(now>=releaseAt||!ready()))releaseDodge();
    unsigned req=requests.exchange(0);
    if(req&8){stop();hold=false;releaseDodge();}
    if(req&4)show=!show;
    if(!nexus||!mumble)return;
    if(!nexus->IsGameplay||mumble->Context.IsInCombat){stop();hold=false;}
    if(req&1){
        if(active||prompt)stop();
        else if(!busy()&&cursor(target)){prompt=true;hold=false;ImGui::OpenPopup("Double-click interval");}
    }
    if((req&2)&&ready()&&mumble->Context.MountIndex==Mumble::EMountIndex::None&&now>=nextDodge&&!dodgePressed){
        usedGameBinds=useGameBinds&&api->GameBinds_IsBound(GB_MoveDodge)&&api->GameBinds_IsBound(GB_MoveJump_SwimUp_FlyUp);
        if(usedGameBinds){api->GameBinds_Press(GB_MoveDodge);api->GameBinds_Press(GB_MoveJump_SwimUp_FlyUp);}
        else{key(dodgeKey,false);key(jumpKey,false);}
        dodgePressed=true;releaseAt=now+30;nextDodge=now+80;
    }
    if(ImGui::BeginPopupModal("Double-click interval",nullptr,ImGuiWindowFlags_AlwaysAutoResize)){
        if(!prompt){ImGui::CloseCurrentPopup();}
        else {
            ImGui::TextUnformatted("Interval in seconds (0.25 to 300):");
            ImGui::InputFloat("##interval",&interval,0.25f,1.f,"%.2f");
            if(ImGui::Button("Start")){
                if(std::isfinite(interval)){interval=std::clamp(interval,0.25f,300.f);save();active=true;prompt=false;nextClick=now;paused=false;ImGui::CloseCurrentPopup();}
            }
            ImGui::SameLine();if(ImGui::Button("Cancel")){stop();ImGui::CloseCurrentPopup();}
        }
        ImGui::EndPopup();
    }
    bool holding=hold.load()&&!prompt;
    if(holding&&active)stop();
    if(active){
        bool blocked=busy()||show||ImGui::GetIO().WantCaptureMouse;
        if(blocked){if(!paused){remaining=nextClick>now?nextClick-now:0;paused=true;}}
        else{
            if(paused){nextClick=now+remaining;paused=false;}
            if(now>=nextClick){click(target);nextClick=now+(ULONGLONG)(interval*1000);}
        }
        ImVec2 p((float)target.x+18,(float)target.y-18);
        ImGui::GetForegroundDrawList()->AddCircle(ImVec2((float)target.x,(float)target.y),12,paused?IM_COL32(255,190,60,255):IM_COL32(100,255,130,255),20,2);
        char label[64];std::snprintf(label,sizeof(label),paused?"Paused":"%.1fs",nextClick>now?(nextClick-now)/1000.f:0.f);
        ImGui::GetForegroundDrawList()->AddText(p,IM_COL32(255,255,255,255),label);
    }else if(holding&&ready()&&!busy()&&!show&&!ImGui::GetIO().WantCaptureMouse&&(!requireMouse||down(VK_LBUTTON))&&now>=nextHold){
        POINT p;if(cursor(p))click(p);nextHold=now+50;
    }
    if(show){if(ImGui::Begin("Inquest Chipset",&show))options();ImGui::End();}
}
static void load(AddonAPI_t* a) {
    api=a;
    ImGui::SetCurrentContext((ImGuiContext*)api->ImguiContext);
    ImGui::SetAllocatorFunctions((void*(*)(size_t,void*))api->ImguiMalloc,(void(*)(void*,void*))api->ImguiFree);
    nexus=(NexusLinkData_t*)api->DataLink_Get(DL_NEXUS_LINK);mumble=(Mumble::Data*)api->DataLink_Get(DL_MUMBLE_LINK);
    std::string dir=api->Paths_GetAddonDirectory("InquestChipset");CreateDirectoryA(dir.c_str(),nullptr);ini=dir+"\\settings.ini";soundPath=dir+"\\double-click.wav";
    auto get=[&](const char* k,int d){return (int)GetPrivateProfileIntA("Inquest",k,d,ini.c_str());};
    interval=std::clamp(get("IntervalMs",1000),250,300000)/1000.f;requireMouse=get("RequireLeftMouse",0)!=0;muted=get("Muted",0)!=0;volume=(float)std::clamp(get("VolumePercent",8),0,100);
    dodgeKey=std::clamp(get("DodgeKey",'V'),1,255);jumpKey=std::clamp(get("JumpKey",VK_SPACE),1,255);useGameBinds=get("UseGameBinds",1)!=0;
    prepareSound();
    // Nexus Keybind_t uses keyboard scan codes, not Windows virtual keys.
    api->InputBinds_RegisterWithStruct(binds[0],onBind,Keybind_t{(uint16_t)MapVirtualKeyA(VK_OEM_COMMA,MAPVK_VK_TO_VSC),false,false,false});
    api->InputBinds_RegisterWithStruct(binds[1],onBind,Keybind_t{(uint16_t)MapVirtualKeyA(VK_OEM_4,MAPVK_VK_TO_VSC),false,false,false});
    api->InputBinds_RegisterWithString(binds[2],onBind,"CTRL+SPACE");
    api->InputBinds_RegisterWithString(binds[3],onBind,"CTRL+SHIFT+I");
    api->WndProc_Register(onWnd);api->GUI_Register(RT_Render,render);api->GUI_Register(RT_OptionsRender,options);
    api->Log(LOGL_INFO,"Inquest Chipset","Nexus port loaded. Ctrl+Shift+I: settings. Clicking is stopped until explicitly activated.");
}
static void unload() {
    api->GUI_Deregister(render);api->GUI_Deregister(options);api->WndProc_Deregister(onWnd);
    for(auto b:binds)api->InputBinds_Deregister(b);
    releaseDodge();stop();hold=false;requests=0;save();PlaySoundA(nullptr,nullptr,0);sound.clear();gameWindow=nullptr;
    api->Log(LOGL_INFO,"Inquest Chipset","Unloaded.");
}
extern "C" __declspec(dllexport) AddonDefinition_t* GetAddonDef() {
    static AddonDefinition_t def{};def.Signature=(uint32_t)-1905210500;def.APIVersion=NEXUS_API_VERSION;def.Name="Inquest Chipset";
    def.Version={0,5,0,1};def.Author="Nekres; local Nexus adaptation";def.Description="Double-click utilities and dodge-jump adapted from the Blish HUD module.";
    def.Load=load;def.Unload=unload;def.Flags=AF_None;def.Provider=UP_None;return &def;
}
