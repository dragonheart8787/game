# Project Nova 長程開發路線圖(Roadmap v1)

> 用途:本文件是 Project Nova 從「戰鬥沙盒 Demo」到「完整願景(程序化開放世界 + AI 輔助內容 + 固定敘事)」的權威路線圖。
> 讀者:開發者本人 + 每一個新開的 Claude Code / agent session。
> 約定:每個 Patch 落地後,更新第 0 節「目前位置」。路線順序可在每個階段結束時重新評估,但**不要在階段中途改順序**。
> 設計來源:所有目標均對應原始設計文件章節(§2.4 可控破壞、§5.1 融合技、§6 生態帶、§7-8 身份劇情與導演系統、§9 AI 生成約束),本路線圖是排程與工程化,**不是重新設計**。

---

## 0. 目前位置(每次 Patch 落地後更新)

- 引擎:UE **5.8**,專案路徑 `D:/UE5.8 project/MyProject`(路徑含空格,一律加引號)
- Git:單一分支 `master`、無 remote、直線歷史
- **Patch 1~6 已完成**:
  - 六個核心類別全部實測驗證(非骨架)
  - Ability Graph 節點系統:Shape(Line/Cone/Sphere)、Path(Instant/Linear)、Spawn(BlockingWall)、Affect(Damage/Block/Slow)、Constraint(TetherToActor);Interact/Cost/Cooldown 為 enum 佔位
  - 三個能力:Slash / Edgewall / Bind,全部 DataAsset 化、參數熱重載驗證通過
  - Identity Override:Full(換裝+鎖 Dash)/ Partial(曝光表)/ Lens(視角切換+Hold)/ Revert(冪等)
  - Story A/B 觸發區、Director mask(Hold/Guide/Punch)、WorldDelta 回寫(revision/hash/flags)
  - 迴歸工具:NovaAbilityTestDriver / NovaIdentityTestDriver / NovaBindTestDriver
- **Patch 7 已完成**(commit `f7c7ddd`,2026-07-12,詳見 `Docs/DevLog-2026-07-12.md`):
  - RendLock 由 `FuseAbilityGraphDefs(Slash, Bind)` 於 runtime 推導,零手寫數值(含 cost/cd);改 Slash DataAsset → RendLock 自動跟隨
  - 暴露並最小侵入處理單一 Affect 限制:`ExtraAffects` 附加陣列(過渡形態,終局遷移歸 P8)
  - NovaFusionTestDriver 16/16 全綠(組合/施放/命中/tether 跟隨/冷卻獨立性),期望值全部 runtime 推導、無寫死調校值
  - 殘留設計債(P8 輸入):Affect+ExtraAffects → 單一 `TArray Affects` 遷移;Spawn/Constraint 仍單槽(Cage / Prism Weave 會踩到)
- **Patch 8 進行中**(2026-07-14):
  - **Task A 已完成**(commit `8c71f47`):Affect 陣列化 —— `FNovaAbilityGraphDef.Affects`(`TArray<FNovaAbilityAffectNode>`)為終局欄位;舊 `Affect`/`ExtraAffects` 降級為隱藏序列化 shim,`UNovaAbilityDataAsset::PostLoad` 經 `MigrateLegacyFields()` 自動收編(冪等;資產重存後 shim 可在後續 patch 刪除)。全部消費端(ExecuteAffect / FuseAbilityGraphDefs / ElementAbilityComponent / FusionTestDriver)已改用 `Affects`。編譯 exit 0;四個 driver PIE 迴歸全綠(Ability ✅ / Identity 全基準吻合 ✅ / Bind 8/8 ✅ / Fusion 18/18 ✅,PASS 數自 16 增至 18 係 P8 測試改寫拆分檢查所致);`DA_Ability_Slash`、`DA_Ability_TestOverride` 均在 log 證實自動遷移
  - 待辦:Cost 節點化、Cooldown 節點化、Niagara soft ref 欄位、修正提供者介面、DataAsset 重存、存讀檔端到端 + SchemaVersion、test driver 合併評估
- **下一步:Patch 8 Task B(Cost 節點化)**

---

## 1. 路線總覽

| 階段 | 內容 | 相對規模 | 結束時的「可停止點」 |
|---|---|---|---|
| A(P7~9) | 核心機制收尾:融合、資料結構終局、Interact/地形互動 | S | 機制完備的戰鬥沙盒 |
| B(P10~12) | 內容管線:VFX 第一條、電影化基建、第一段真敘事 | M | 可對外展示的 Vertical Slice |
| C | 結構擴展:敵人 AI、HUD、能力塑形輸入、第二生態帶 | M | 有敵人、有手感深度的 demo |
| E | 世界反應系統:WorldState v2、規則引擎、世界真的會變 | M | 行為有後果的世界骨架 |
| F | 互動與破壞:互動分類、可控破壞 v2、NPC 對話 | M~L | 可探索可互動的世界 |
| G | 固定主線+支線:章節框架、第一章、支線模板 | L | **完整的章節制單機遊戲** |
| H | AI 生成劇情:Validator、LLM 支線生成、品質分層 | M | 環境支線自動補充的世界 |
| I | 程序化開放世界:生態帶規則、PCG、錨點+填充 | XL | 完整願景 |
| D | 多人 / 玩家自建世界 / 自製引擎 | — | **明確擱置,不排期** |

規模說明:S=數個 patch 內;M=一段密集期;L=以「內容量」為主的長期投入;XL=需要重新評估人力與範圍。一人 + AI agent 的前提下,**每個停止點都是合法的出貨/暫停點**,路線圖的價值是讓每一步都踩在完整狀態上,不是承諾全部做完。

---

## 2. 排序邏輯(五條原則)

1. **風險先行**:架構未知數(融合、資料結構形狀)最先做,內容量問題(劇情、資產)靠後——內容晚做成本不變,架構晚改成本翻倍。
2. **管線先於內容**:VFX/電影化/對話 UI 這些「生產工具」先走通一條,再量產內容。
3. **世界反應先於互動**:F 階段的破壞/互動結果需要 E 階段的持久化地基才有意義。
4. **固定內容先於 AI 生成**:G 的支線模板就是 H 的 AI 的詞彙表;沒有模板,AI 生成無從約束。
5. **宏觀手工、微觀程序**:世界大結構(生態帶佈局、國界、主線錨點)永遠手工;程序化與 AI 只負責填充與變體。

---

## 3. 階段詳細規格

### 階段 A(Patch 7~9):核心機制收尾

#### Patch 7:融合機制(Slash × Bind → RendLock)【已規劃】
- 目標:證明 Ability Graph 撐得住「兩個能力組合成第三個」,不是複製貼上假融合。
- 關鍵約束:**若發現 Affect 需要從單一效果改成陣列才能疊加,先用最小侵入方式驗證概念,結構改動留給 Patch 8 統一做**。
- 驗收:RendLock 同時具備 Slash 傷害 + Bind 降速/鎖鏈;三能力冷卻互不干擾;誠實回報架構限制(若有)。

#### Patch 8:資料結構終局設計 + Cost/Cooldown 節點化 + 存讀檔端到端
這是「一次遷移到位」的 patch。`FNovaAbilityGraphDef` 在 P7 / P8 / P10 / 階段 I 四處都會被動到,**在這裡一次設計完終局形狀**,避免資產遷移三次:
- Affect:單一節點 → `TArray<FNovaAbilityAffectNode>`(P7 的疊加需求)
- 新增 Cost 節點(目前 EnergyCost 頂層欄位收編;預留未來 health/材料成本型態)
- 新增 Cooldown 節點(預留共享冷卻組 group id,融合技與母技的冷卻關係在這裡定義)
- Spawn/Affect 節點加 `TSoftObjectPtr<UNiagaraSystem>` + attach socket 欄位(P10 要用,現在先留欄位不接資產)
- Runtime 增加一個「修正提供者」介面(`INovaAbilityModifierProvider` 之類):執行時查詢外部修正(階段 I 的生態帶規則就從這裡插入,屆時**不需要再改結構**)
- 資產遷移:現有 4 個 DataAsset 重存;沿用 P6 的教訓(enum 只能往後加,不能插中間)
- 存讀檔端到端:關遊戲 → 重開 → WorldState(flags/revision/hash)完整還原;**加入 SchemaVersion 欄位 + 遷移函式框架**(之後每階段擴 schema 都靠它)
- 順手評估:三個 test driver 合併成一個參數化 `NovaRegressionDriver`(結構改動後三個 driver 都要跟著改,是合併的好時機)
- 驗收:全部既有迴歸重跑全綠;v1 存檔可被 v2 讀取。

#### Patch 9:Interact 節點 + 可控地形互動最小版 + 專用測試關卡
- Gameplay Tags 分類:`Nova.Interact.Destructible / AbilityReactive / Movable / Talkable / Lootable`
- Interact 節點:命中時查目標 tag → 分派行為;最小案例 = Slash 打 Destructible 牆 → 換破損 mesh + decal + 寫入 WorldDelta(**第一次物件級持久化**:壞掉的牆在存檔裡保持壞的)
- 建立專用測試關卡(取代一直借用的 Lvl_ThirdPerson):地面、tag 牆(可破壞/反應/普通)、anchor、dummy 生成點、story 觸發區——之後所有迴歸都跑這張圖
- 驗收:破壞 → 存檔 → 重開 → 牆仍是壞的;未 tag 物件不受影響。

> **彈性插槽 C0(可提前)**:能力塑形輸入層(charge 蓄力延長 range、release timing、target surface 指定)。這是遊戲核心幻想「塑形而非按技能」的手感所在,目前只有 API 沒有輸入。若想優先驗證手感,可在 P9 之後立刻做;否則排在階段 C。→ 見決策點 D1。

### 階段 B(Patch 10~12):內容管線打通

#### Patch 10:VFX 管線第一條(以 Slash 為載體)
- 另一台資產機產出第一份 Niagara 可用素材(flipbook 或 mesh+材質)→ `_workspace/raw_vfx` → **實作 `import_flipbook.py`**(目前只是規劃)→ `Content/VFX`
- Slash 的 Spawn/Affect 節點掛上 Niagara soft ref(欄位 P8 已備好)
- DrawDebug 不刪除,改掛在 CVar 後面(`nova.DebugAbilityDraw`)——迴歸 driver 依賴 log 斷言,視覺換裝不影響驗證
- 產出 `Docs/AssetPipeline.md`:整條路的操作紀錄,之後其他能力照抄流程
- 風險:第一次走通必超時,**目標是「走通」不是「好看」**,分清楚。

#### Patch 11:電影化基建(Director × Sequencer)
- Director beat 驅動 Level Sequence:Hold beat 播放鏡頭 sequence、Punch beat 觸發 time dilation 慢動作 + Camera Shake 資產
- 最小字幕/對白條 UI(UMG;若要長期投資可評估 CommonUI)
- 音效 stub:MetaSounds 佔位(施放音、命中音、story stinger)——音效從這裡開始有掛點,不再完全缺席
- FText 紀律從此建立(P4 曾發現 NSLOCTEXT namespace 遺失;UI 起步就走可本地化路線,避免日後回頭補)
- 驗收:Story A 的四個 beat 有真實運鏡與音效節奏,不再只是 debug log。

#### Patch 12:第一段真敘事(Chapter 0)+ 角色資產(視另一台機器進度)
- 用已驗證的全套系統做一段 3~5 分鐘 Type-A 內容(建議用 Lens 開場 → Partial 潛入 → 小型 Full Override 高潮的縮小版)
- Lyn 模型/動畫若資產管線已產出則替換 Quinn;否則保持佔位,**角色資產是平行軌,不阻塞主線**
- 驗收:一個不認識這個專案的人可以坐下來玩完這段並大致理解遊戲是什麼。

### 階段 C:結構擴展
- **C1 敵人 AI v1**:Dummy → `ANovaEnemyBase`;StateTree(UE5 原生,較 BT 輕)+ 視覺感知;狀態:idle/patrol/chase/attack/被 Bind 減速時的 stunned 反應
- **C2 HUD v1**:血量/能量條、能力冷卻圖示——玩家面向的數值離開 debug overlay(overlay 保留給開發)
- **C3 能力塑形輸入層**(若未提前為 C0):charge、release timing、target surface;這是高手上限的來源,做完後 Slash 的 arc/range 真正由玩家即時塑形
- **C4 第二生態帶迷你關**:玻璃荒原規則(Line 形狀能力有反射機率)——**第一次驗證「生態帶規則影響能力」**,直接預演階段 I 的 I1,用 P8 預留的 ModifierProvider 介面插入,不改結構

### 階段 E:世界反應系統(「你的行為影響整個世界」)

#### E1:WorldState schema v2 + 版本化
```
FNovaWorldStateV2 {
  int32  SchemaVersion = 2;
  TMap<FName,int32>            EventFlags;        // 既有
  TMap<FName,int32>            FactionRelations;  // -10000..10000 定點數(浮點進 hash 不穩定,刻意用 int)
  TMap<FName,ENovaZoneState>   Zones;             // Open/Locked/Occupied/Destroyed
  TMap<FGuid,FNovaNpcRecord>   NpcRegistry;       // 存活、態度、所在 zone
  TArray<FNovaChoiceRecord>    PlayerHistory;     // 關鍵選擇(choice id, story id, revision)
  uint64                       WorldSeed;         // 階段 I 用,現在先留
  TArray<FNovaObjectDelta>     ObjectDeltas;      // 稀疏物件狀態(P9 的破壞牆收編到這)
}
```
- 遷移:v1 存檔(P8 產)→ v2 讀取測試必過;journal 維持 append-only,hash 覆蓋全狀態
- **Registry 模式從這裡確立**:NpcRegistry / Zone 表 /(F1 的)Interactable 註冊表 = 之後規則引擎、Validator、程序化生成**讀同一份真相**。這是把五個長程目標串起來的核心架構決策。

#### E2:反應規則引擎
- `UNovaWorldRuleAsset`(DataAsset):條件(flag/faction/zone 查詢,AND/OR)→ 效果(改 NPC 態度、切 Data Layer、開關 spawner、改對話分支指標)
- 觸發時機:WorldDelta apply 時 / 進入 zone 時 / story 開始時——**事件驅動,不逐 tick 掃描**
- 衝突解法:顯式 Priority + 資產名決定平手(確定性);**每次規則觸發寫入 journal**,debug overlay 加一頁「最近 N 條規則觸發」
- 設計鐵律:規則**讀狀態、設狀態,絕不累加**(保證重放冪等,存讀檔後規則重評估不會疊加出錯)
- UE 原生掛點:zone 狀態 ↔ World Partition **Data Layers**(Harbor Destroyed → 啟用 Harbor_Ruined layer)——用引擎機制做世界變體,便宜且可視化編輯

#### E3:Story delta 真格化
- Story A 完成 → faction delta + zone 狀態改變 → 規則引擎 → 下次進入該區:不同 NPC 反應/敵人配置
- 驗收:腳本化 driver 跑 story → 存檔 → 重讀 → 世界差異持久且規則重評估冪等。

### 階段 F:互動與破壞(「可破壞、跟所有人物和物品互動」)

「所有」的可執行定義:**不是每個物件有獨特互動,而是每個物件屬於某個互動類別**。

#### F1:互動分類系統
- `UNovaInteractableComponent`:tags + 互動動詞(Examine/Use/Talk/Loot/Destroy/AbilityReact)+ hooks;BeginPlay 註冊進 `UNovaInteractionSubsystem`(**進 Registry,供 Validator 與能力查詢**)
- 「萬物可互動」的底線:所有擺放物件至少有 Examine(一行 flavor text)——用 Editor Python 稽核腳本掃出未 tag 物件,批次補預設
- 工具:`Tools/audit_interactables.py`

#### F2:可控破壞 v2(把 §2.4 的決策法典化)
| 層級 | 手段 | 適用 | 成本 |
|---|---|---|---|
| T1 | 換 mesh(完好→破損→殘骸)+ decal + VFX | 90% 物件 | 低 |
| T2 | 預先破碎的 Geometry Collection(Chaos),物理短暫啟用後凍結清理 | 英雄時刻(boss 進場砸牆) | 中 |
| T3 | 執行期任意物理破碎 | **禁止**(§2.4 原決策) | — |
- 持久化:`FNovaObjectDelta{ObjectGuid, NewStateIndex}`,只記 delta、預設完好(控制存檔膨脹);關卡物件用穩定 actor GUID
- 能力 × 材質互動矩陣(DataAsset 表):`材質 tag × 能力族 → 反應`(能量鎖鏈打 Conductive → 電弧+暈眩;Slash 打 Glass → 碎裂)——地形互動從單點案例升級為系統

#### F3:NPC 對話基礎
- 對話樹 DataAsset(節點、讀 WorldState 的條件、發 WorldDelta 的效果)+ UMG 對話 UI + Talkable NPC(StateTree idle)
- 態度閘門:NpcRegistry 態度 + faction 關係餵入條件
- 範圍護欄:純文字、無配音、無口型——內容打磨留到 G 之後。

### 階段 G:固定主線+支線(內容生產期)

#### G1:章節框架
- `UNovaChapterAsset{前置條件(WorldState 查詢), StoryGraph(beats+分支), 結果(WorldDelta 集), 解鎖}`;章節進度入 WorldState;debug 章節選單

#### G2:第一章(Full Override 主線)
- 8~15 分鐘完整章:Lens 開場情報 → Partial 潛入(曝光表真格運作)→ Full Override 高潮戰鬥(不同 loadout)→ 結局寫入世界、回到 hub 可見差異
- **這一章就是全架構的整合測試**——每個已建系統都被真實內容咬合一次
- 劇本主導權在你;agent 的角色是把劇本工程化(beats/條件/delta),不是代寫故事 → 見決策點 D4

#### G3:支線模板系統
- `FNovaSideQuestTemplate{觸發條件, 參數化 beat 序列, 所需實體角色(「X 陣營的一個 NPC、位於 Y 類 zone」——角色而非具體 ID), Delta 結果}`
- 手寫 3~5 個模板,先手動實例化跑通
- **參數化即 AI 填空面**:階段 H 的 AI 生成本質 = 往這些模板綁定 Registry 裡的實體。

### 階段 H:AI 生成劇情(刻意排在 AI 建世界之前——可驗證性高)

#### H1:Validator(整個階段的核心,比生成本身重要)
- 輸入:候選支線 JSON(模板 id + 參數綁定)+ WorldState 快照;輸出:pass | fail(reasons[])
- 檢查:實體白名單(引用的 NPC/zone/faction 必須在 Registry)、存活、可達(zone Open)、faction 前置一致、與 PlayerHistory 不矛盾、長度/beat 預算、禁用組合
- **白名單制,不是黑名單制**:生成器只能綁定 Registry 暴露的實體;自由文字僅限展示用,永不作為引用
- 純函式、確定性、可單元測試——建一組「精心構造的違規案例」測試集;fail → fallback 模板(§9.2 原要求),**生成失敗永不阻塞遊戲**

#### H2:生成器整合(本地 LLM)
- 流程:世界事件觸發 → 組**精簡**上下文(相關狀態切片,不是整個 state)→ LLM 產模板參數 JSON → JSON schema 驗證 → Validator → 生成或 fallback
- 延遲策略:**預生成佇列**(zone 載入時背景生成 2~3 個候選),絕不讓遊戲等推理
- 部署選項(→ 決策點 D2):同機推理會跟 UE 搶 GPU;可行替代 = 資產機經區網提供推理、或只在載入畫面生成
- 展示文字護欄:長度上限、不得出現 Registry 以外的新名字

#### H3:品質分層與遙測
- AI 支線在 UI 標記為「環境支線」,與手工主線區隔(管理玩家預期);記錄 Validator 拒絕率——拒絕率高時**收緊 prompt/模板,不放鬆 Validator**
- 鐵律:AI 生成內容永不閘門主線進度。

### 階段 I:程序化開放世界(最後,且刻意重新定義)

誠實前提:「AI 即時生成整個開放世界」目前仍是研究題。可達成的版本 = **宏觀手工 + 微觀程序 + AI 供變體**,而你的生態帶結構(§6)正好是這個模式的完美骨架。

- **I0 世界宏觀結構(設計工作)**:生態帶佈局、國界、主線錨點位置——手繪世界聖經 + 粗粒度 zone graph 資產;程序化與 AI 永不觸碰宏觀
- **I1 生態帶規則 DataAsset**:天氣組、能量修正(玻璃荒原:Line 形狀反射;磁暴紅林:tether 強度 ×1.5 + 控制噪聲)、危險生成表、移動參數——經 P8 的 ModifierProvider 介面插入能力 runtime,**零結構改動**(C4 已預演)
- **I2 PCG(UE 原生 Procedural Content Generation)逐帶生成**:輸入 = seed + zone graph cell + 生態帶資產;輸出 = 散佈、廢墟擺放(從變體庫按 tag 取)、資源/遭遇點。**確定性要求**:seed 存 WorldState;同 seed 重生成必須逐位元一致(測試:生成兩次、hash 擺放結果比對)——先從最小切片開始:**一個 cell、一個變體庫**
- **I3 資產變體庫(資產機管線規模化)**:20 種廢墟建築、15 種水晶簇這類批次;命名/tag 約定讓 PCG 可按 tag 消費;→ 決策點 D3(美術方向先鎖)
- **I4 錨點+填充整合**:手工錨點區用 PCG 排除體積壓進生成結果;World Partition 串流;持久化 = **seed(可重生成)+ ObjectDeltas(記變化)雙層模型**——驗收:打壞程序化生成的牆 → 存檔 → 重開 → 走回去牆還是壞的
- **I5 世界一致性驗證(H1 Validator 的空間延伸)**:生成營地不得落在 Locked zone;程序化生成的 NPC 用確定性 GUID(seed+cell+index 導出)**自動註冊進 NpcRegistry**——程序化世界餵進與劇情/AI 相同的 Registry,閉環完成。

---

## 4. 跨階段基礎設施(每階段都要回頭看的清單)

1. **Schema 版本化**:P8 起建立;E1/F2/I2 各擴一次 schema;每次擴充 = 一個遷移函式 + 一個舊檔讀取迴歸測試
2. **測試 driver 演進**:P8 合併為單一參數化 driver → 階段 E 起考慮 JSON 腳本化測試序列(手寫 driver 撐不到 E~I 的量)
3. **Registry 單一真相**:E1 確立;F1/H1/I5 全部插同一組 Registry,不允許旁路資料源
4. **各階段 debug 工具**:E 規則觸發頁 / F 互動稽核腳本 / G 任務狀態檢視器 / H Validator 報告 / I PCG 確定性 hash 檢查
5. **效能預算**:B 結束時定 frame time 目標;之後每階段結束重測一次(串流 + 規則評估是 E~I 的主要新增成本)
6. **本地化紀律**:P11 起所有玩家可見文字走 FText;正式本地化決策延到 G 之後

---

## 5. 風險清單

**近期(A~B)**:融合暴露 Affect 結構限制(對策:P7 最小驗證、P8 統一改)/ VFX 管線首跑超時(對策:「走通」與「好看」分兩目標)/ 資料結構終局設計思慮不足導致再遷移(對策:P8 設計時對照本文件 E1/I1 的欄位需求)

**中期(E~G)**:規則蔓延不可調試(對策:journal + 觸發日誌頁 + 冪等鐵律)/ 存檔膨脹(對策:稀疏 delta、預設完好)/ 章節內容周邊系統低估(對話 UI、字幕、運鏡——P11 提前建基建即為此)/ 內容量是 G 的真正瓶頸,不是技術

**遠期(H~I)**:Validator 覆蓋率永遠不夠(對策:白名單制從根本封死)/ 推理搶 GPU(對策:D2 提前定案)/ PCG × 持久化雙層模型複雜(對策:最小切片先行)/ 「開放世界」的真相是內容密度問題(對策:第一目標 = **一條生態帶做好**,不是六條)

**系統性**:一人 + agent 的範圍管理——每個停止點都是完整狀態,允許在任何停止點結束或長期停留;每階段開頭重新評估後續順序。

---

## 6. 待決策點(只有你能決定,提前標記)

| # | 決策 | 何時需要 | 影響 |
|---|---|---|---|
| D1 | 能力塑形輸入(charge/release/表面指定)要不要提前到 P9 後(C0)? | P9 結束前 | 核心手感 vs 系統推進的優先序 |
| D2 | 本地 LLM 跑哪:同機 / 資產機經區網 / 只在載入畫面生成? | 階段 H 開工前 | GPU 資源分配、架構 |
| D3 | 生態帶變體庫的美術方向鎖定(風格指南給資產管線) | 階段 I3 前 | 資產機管線的產出方向 |
| D4 | 第一章劇本(agent 工程化、你主導故事) | G2 前 | 敘事所有權 |
| D5 | 多人議題何時重開(建議:I 穩定前不碰;WorldDelta 單一 apply 點的現有設計已是最便宜的保險) | 不早於 I | 架構長期走向 |

---

## 7. 明確擱置項(D 階段)

多人、玩家自建世界、AI 即時生成完整世界、自製引擎——**全部不排期**。單機章節制遊戲(G 停止點)交付之前,任何投入這些方向的工時都是火力分散。自製引擎維持原始文件 §12 的定位:長期方向,UE5 驗證玩法優先。
