// =========================================================================
// Smart Nutrition Scale — Full-System Digital Twin Application Logic
// Orchestrates ESP32 Firmware simulation & Android Companion App
// =========================================================================

(function() {
  // -------------------------------------------------------------
  // Global Hardware State (ESP32 Scale)
  // -------------------------------------------------------------
  const STATE_IDLE_SELECT = 0;
  const STATE_TARE_PROMPT = 1;
  const STATE_WEIGHING    = 2;
  const STATE_RESULT      = 3;

  let currentState = STATE_IDLE_SELECT;
  let selectedIndex = 0;
  let rawScaleWeight = 0.0;
  let tareOffset = 0.0;
  let isWifiConnected = true;
  let calFactor = -2180.0; // Typical 1kg load cell sensitivity (~2180 counts/g)
  const MAX_CAPACITY = 1000.0; // 1kg

  // Settling logic
  const SETTLING_WINDOW = 6;
  const SETTLING_TOLERANCE = 0.5;
  let weightHistory = [];
  let currentSettledWeight = 0.0;
  let isSettled = false;

  // Running Session Totals (Accumulates across items)
  let session = {
    itemCount: 0,
    calories: 0.0,
    protein: 0.0,
    fat: 0.0,
    carbs: 0.0,
    grams: 0.0,
    items: []
  };

  // Active Shortlist on ESP32 Scale (Max 20 items, stored in Flash NVS)
  let activeShortlist = [
    { slot: 0, name: "Mustard Oil", cal100: 884.0, protein100: 0.0, fat100: 100.0, carb100: 0.0 },
    { slot: 1, name: "Soya Bean (Tofu)", cal100: 141.0, protein100: 14.5, fat100: 8.5, carb100: 2.5 },
    { slot: 2, name: "White Rice (Raw)", cal100: 356.0, protein100: 7.9, fat100: 0.5, carb100: 78.2 },
    { slot: 3, name: "Paneer", cal100: 289.0, protein100: 18.3, fat100: 20.8, carb100: 3.4 },
    { slot: 4, name: "Moong Dal", cal100: 334.0, protein100: 24.0, fat100: 1.3, carb100: 56.5 }
  ];

  // Active Shortlist in Companion Phone App (Max 20 items)
  let phoneShortlist = [
    { slot: 0, name: "Mustard Oil", cal100: 884.0, protein100: 0.0, fat100: 100.0, carb100: 0.0 },
    { slot: 1, name: "Soya Bean (Tofu)", cal100: 141.0, protein100: 14.5, fat100: 8.5, carb100: 2.5 },
    { slot: 2, name: "White Rice (Raw)", cal100: 356.0, protein100: 7.9, fat100: 0.5, carb100: 78.2 },
    { slot: 3, name: "Paneer", cal100: 289.0, protein100: 18.3, fat100: 20.8, carb100: 3.4 },
    { slot: 4, name: "Moong Dal", cal100: 334.0, protein100: 24.0, fat100: 1.3, carb100: 56.5 }
  ];

  // DOM Elements
  // -------------------------------------------------------------
  const tftScreen = document.getElementById('tft-screen');
  const tftHeaderTitle = document.getElementById('tft-header-title');
  const tftWifiBadge = document.getElementById('tft-wifi-badge');
  const globalWifiDot = document.getElementById('global-wifi-dot');
  const globalWifiLabel = document.getElementById('global-wifi-label');

  // TFT Views
  const viewIdle = document.getElementById('view-idle');
  const viewTare = document.getElementById('view-tare');
  const viewWeighing = document.getElementById('view-weighing');
  const viewResult = document.getElementById('view-result');
  const tftToast = document.getElementById('tft-toast');
  const toastTitle = document.getElementById('toast-title');
  const toastMsg = document.getElementById('toast-msg');

  // Idle view fields
  const idleLiveWeight = document.getElementById('idle-live-weight');
  const foodSlotLabel = document.getElementById('food-slot-label');
  const idleFoodName = document.getElementById('idle-food-name');
  const idleMacroLine1 = document.getElementById('idle-macro-line1');
  const idleMacroLine2 = document.getElementById('idle-macro-line2');
  const idleSessionTitle = document.getElementById('idle-session-title');
  const idleSessionSub = document.getElementById('idle-session-sub');

  // Under-Phone Live Session Display Elements
  const underPhoneSessionTitle = document.getElementById('under-phone-session-title');
  const underPhoneSessionSub = document.getElementById('under-phone-session-sub');
  const underPhonePPill = document.getElementById('under-phone-p-pill');
  const underPhoneFPill = document.getElementById('under-phone-f-pill');
  const underPhoneCPill = document.getElementById('under-phone-c-pill');
  const underPhoneWtPill = document.getElementById('under-phone-wt-pill');
  const underPhoneHistoryCount = document.getElementById('under-phone-history-count');
  const underPhoneSessionItems = document.getElementById('under-phone-session-items');
  const underPhoneBtnReset = document.getElementById('under-phone-btn-reset');

  // Tare view fields
  const tareFoodName = document.getElementById('tare-food-name');
  const tareLiveWeight = document.getElementById('tare-live-weight');

  // Weighing view fields
  const weighingFoodName = document.getElementById('weighing-food-name');
  const weighingLiveWeight = document.getElementById('weighing-live-weight');
  const settlingBadge = document.getElementById('settling-badge');

  // Result view fields
  const resultFoodName = document.getElementById('result-food-name');
  const resultWeightLabel = document.getElementById('result-weight-label');
  const resCal = document.getElementById('res-cal');
  const resProt = document.getElementById('res-prot');
  const resFat = document.getElementById('res-fat');
  const resCarb = document.getElementById('res-carb');

  // Hardware Rotary Knob & Platform
  const rotaryKnob = document.getElementById('rotary-knob');
  const btnCcw = document.getElementById('btn-ccw');
  const btnCw = document.getElementById('btn-cw');
  const scalePlatter = document.getElementById('scale-platter');
  const onScaleLabel = document.getElementById('on-scale-label');
  const weightSlider = document.getElementById('weight-slider');
  const sliderWeightVal = document.getElementById('slider-weight-val');

  // Presets
  const btnPlaceBowl = document.getElementById('btn-place-bowl');
  const btnAddOil = document.getElementById('btn-add-oil');
  const btnAddPaneer = document.getElementById('btn-add-paneer');
  const btnAddRice = document.getElementById('btn-add-rice');
  const btnOverload = document.getElementById('btn-overload');
  const btnRemoveAll = document.getElementById('btn-remove-all');

  // Serial Console
  const serialOutput = document.getElementById('serial-output');
  const serialInput = document.getElementById('serial-input');
  const btnSendSerial = document.getElementById('btn-send-serial');
  const btnClearSerial = document.getElementById('btn-clear-serial');

  // Phone App DOM
  const phoneCounterPill = document.getElementById('phone-counter-pill');
  const phoneFoodList = document.getElementById('phone-food-list');
  const phoneSearchInput = document.getElementById('phone-search-input');
  const dbCountLabel = document.getElementById('db-count-label');
  const phoneShortlistItems = document.getElementById('phone-shortlist-items');
  const capacityNumberLabel = document.getElementById('capacity-number-label');
  const capacityBarFill = document.getElementById('capacity-bar-fill');
  const phoneBtnClearAll = document.getElementById('phone-btn-clear-all');

  // Custom Food Modal Elements
  const btnOpenCustomModal = document.getElementById('btn-open-custom-modal');
  const customFoodModal = document.getElementById('custom-food-modal');
  const btnCloseModal = document.getElementById('btn-close-modal');
  const btnSaveCustomFood = document.getElementById('btn-save-custom-food');
  const customBrandInput = document.getElementById('custom-brand-input');
  const customNameInput = document.getElementById('custom-name-input');
  const customGroupInput = document.getElementById('custom-group-input');
  const customCalInput = document.getElementById('custom-cal-input');
  const customProtInput = document.getElementById('custom-prot-input');
  const customFatInput = document.getElementById('custom-fat-input');
  const customCarbInput = document.getElementById('custom-carb-input');
  const customShortlistCheck = document.getElementById('custom-shortlist-check');

  // Phone Wi-Fi Tab
  const syncStatusIcon = document.getElementById('sync-status-icon');
  const syncConnStatus = document.getElementById('sync-conn-status');
  const scaleHostInput = document.getElementById('scale-host-input');
  const btnTestConn = document.getElementById('btn-test-conn');
  const syncDescText = document.getElementById('sync-desc-text');
  const syncProgressBar = document.getElementById('sync-progress-bar');
  const syncProgressFill = document.getElementById('sync-progress-fill');
  const btnPhoneSyncNow = document.getElementById('btn-phone-sync-now');
  const protocolLog = document.getElementById('protocol-log');

  // Bottom Nav Tabs
  const navTabs = document.querySelectorAll('.nav-tab');
  const tabDatabase = document.getElementById('tab-database');
  const tabShortlist = document.getElementById('tab-shortlist');
  const tabSync = document.getElementById('tab-sync');
  const appBarTitle = document.getElementById('app-bar-title');

  // -------------------------------------------------------------
  // Boot & Initialization
  // -------------------------------------------------------------
  function boot() {
    logSerial("==================================================");
    logSerial("       SMART NUTRITION SCALE FIRMWARE BOOT        ");
    logSerial("==================================================");
    logSerial("[DISPLAY] TFT_eSPI ILI9341 240x320 initialized.");
    logSerial("[SHORTLIST] Initialized with " + activeShortlist.length + " active items.");
    logSerial("[SCALE] Initializing HX711... Cal Factor: " + calFactor.toFixed(2));
    logSerial("[SCALE] Tared (zeroed) successfully.");
    logSerial("[ENCODER] Rotary encoder interrupts initialized.");
    logSerial("[WIFI] Connecting to network...");
    logSerial("[WIFI] Connected! IP: 192.168.1.150 (RSSI -56 dBm)");
    logSerial("[MDNS] Responder started: http://smartscale.local");
    logSerial("[HTTP] WebServer listening on port 80 (REST API ready)");
    logSerial("[SYSTEM] Initialization complete. Standalone scale ready.");
    logSerial("[SYSTEM] Send 'CAL' in Serial Monitor anytime to calibrate load cell.\n");

    loadFoodsData();
    renderTft();
    updateSessionDisplay();
    setupEventListeners();
    setupRotaryButton();
  }

  // -------------------------------------------------------------
  // Scale Physics & Weight Processing
  // -------------------------------------------------------------
  function getNetWeight() {
    let net = rawScaleWeight - tareOffset;
    if (Math.abs(net) < 0.2) net = 0.0;
    return net;
  }

  function setScaleWeight(grams, label = null) {
    rawScaleWeight = Math.max(0, grams);
    weightSlider.value = Math.min(1000, rawScaleWeight);
    sliderWeightVal.textContent = rawScaleWeight.toFixed(1);

    if (rawScaleWeight > MAX_CAPACITY) {
      onScaleLabel.textContent = `⚠️ OVERLOAD (>1000g) (${rawScaleWeight.toFixed(1)}g)`;
      showToast("OVERLOAD!", "> 1000g MAX LOAD", true);
      logSerial(`[SCALE WARNING] Overload detected: ${rawScaleWeight.toFixed(1)}g exceeds 1kg load cell limit!`);
    } else if (label) {
      onScaleLabel.textContent = label + " (" + rawScaleWeight.toFixed(1) + "g)";
    } else {
      onScaleLabel.textContent = rawScaleWeight > 0 ? "Items on Scale (" + rawScaleWeight.toFixed(1) + "g)" : "Scale Platform Empty";
    }

    scalePlatter.classList.toggle('depressed', rawScaleWeight > 0);
    scalePlatter.style.setProperty('--load-pct', Math.min(100, Math.max(0, (rawScaleWeight / MAX_CAPACITY) * 100)) + '%');

    // Feed settling window
    feedSettling(getNetWeight());
    updateLiveDisplays();
  }

  function feedSettling(weight) {
    weightHistory.push(weight);
    if (weightHistory.length > SETTLING_WINDOW) {
      weightHistory.shift();
    }

    if (weightHistory.length >= SETTLING_WINDOW && weight >= 1.0) {
      let min = Math.min(...weightHistory);
      let max = Math.max(...weightHistory);
      if ((max - min) <= (SETTLING_TOLERANCE * 2.0)) {
        isSettled = true;
        currentSettledWeight = weightHistory.reduce((a, b) => a + b, 0) / SETTLING_WINDOW;
      } else {
        isSettled = false;
      }
    } else {
      isSettled = false;
    }

    // Auto-advance in WEIGHING state once settled
    if (currentState === STATE_WEIGHING && isSettled) {
      settlingBadge.textContent = "STABLE / SETTLED";
      settlingBadge.classList.add('settled');

      setTimeout(() => {
        if (currentState === STATE_WEIGHING) {
          currentState = STATE_RESULT;
          renderTft();
        }
      }, 500);
    }
  }

  // -------------------------------------------------------------
  // TFT Display Engine
  // -------------------------------------------------------------
  function renderTft() {
    // Hide all views
    viewIdle.classList.add('hidden');
    viewTare.classList.add('hidden');
    viewWeighing.classList.add('hidden');
    viewResult.classList.add('hidden');

    tftWifiBadge.textContent = isWifiConnected ? "WIFI" : "OFF";
    tftWifiBadge.classList.toggle('active', isWifiConnected);

    let net = getNetWeight();
    let currentFood = activeShortlist[selectedIndex] || { name: "No Item", cal100: 0, protein100: 0, fat100: 0, carb100: 0 };

    switch (currentState) {
      case STATE_IDLE_SELECT:
        tftHeaderTitle.textContent = "SMART NUTRITION SCALE";
        viewIdle.classList.remove('hidden');

        idleLiveWeight.textContent = net.toFixed(1) + " g";
        if (activeShortlist.length === 0) {
          foodSlotLabel.textContent = "SHORTLIST EMPTY (0/20)";
          idleFoodName.textContent = "Add Foods in Phone App";
          idleMacroLine1.textContent = "Tap ★ in Phone Database";
          idleMacroLine2.textContent = "to load items into scale";
        } else {
          foodSlotLabel.textContent = `FOOD ITEM (${selectedIndex + 1}/${activeShortlist.length})`;
          idleFoodName.textContent = currentFood.name;
          idleMacroLine1.textContent = `Per 100g: ${Math.round(currentFood.cal100)} kcal | P:${currentFood.protein100.toFixed(1)}g`;
          idleMacroLine2.textContent = `F:${currentFood.fat100.toFixed(1)}g | C:${currentFood.carb100.toFixed(1)}g`;
        }

        updateSessionDisplay();
        break;

      case STATE_TARE_PROMPT:
        tftHeaderTitle.textContent = "STEP 1: TARE CONTAINER";
        viewTare.classList.remove('hidden');

        tareFoodName.textContent = currentFood.name;
        tareLiveWeight.textContent = rawScaleWeight.toFixed(1) + " g";
        break;

      case STATE_WEIGHING:
        tftHeaderTitle.textContent = "STEP 2: ADD FOOD";
        viewWeighing.classList.remove('hidden');

        weighingFoodName.textContent = currentFood.name;
        weighingLiveWeight.textContent = net.toFixed(1) + " g";

        settlingBadge.textContent = isSettled ? "STABLE / SETTLED" : "WAITING TO SETTLE...";
        settlingBadge.classList.toggle('settled', isSettled);
        break;

      case STATE_RESULT:
        tftHeaderTitle.textContent = "NUTRITION COMPUTED";
        viewResult.classList.remove('hidden');

        resultFoodName.textContent = currentFood.name;
        resultWeightLabel.textContent = `Weighed: ${currentSettledWeight.toFixed(1)} g`;

        // Strict Formula: (weight / 100.0) * value_per_100g
        let factor = currentSettledWeight / 100.0;
        let cals = factor * currentFood.cal100;
        let prot = factor * currentFood.protein100;
        let fat = factor * currentFood.fat100;
        let carbs = factor * currentFood.carb100;

        resCal.textContent = Math.round(cals);
        resProt.textContent = prot.toFixed(1);
        resFat.textContent = fat.toFixed(1);
        resCarb.textContent = carbs.toFixed(1);
        break;
    }
  }

  function updateLiveDisplays() {
    let net = getNetWeight();
    if (currentState === STATE_IDLE_SELECT) {
      idleLiveWeight.textContent = net.toFixed(1) + " g";
    } else if (currentState === STATE_TARE_PROMPT) {
      tareLiveWeight.textContent = rawScaleWeight.toFixed(1) + " g";
    } else if (currentState === STATE_WEIGHING) {
      weighingLiveWeight.textContent = net.toFixed(1) + " g";
    }
  }

  function showToast(title, msg, isReset = false) {
    toastTitle.textContent = title;
    toastMsg.textContent = msg;
    tftToast.classList.remove('hidden');
    tftToast.classList.toggle('reset', isReset);
    setTimeout(() => {
      tftToast.classList.add('hidden');
    }, 1200);
  }

  // -------------------------------------------------------------
  // Rotary Encoder Interactions
  // -------------------------------------------------------------
  let knobAngle = 0;
  function rotateKnob(delta) {
    knobAngle += delta * 20;
    rotaryKnob.style.transform = `rotate(${knobAngle}deg)`;

    if (currentState === STATE_IDLE_SELECT) {
      if (activeShortlist.length > 0) {
        selectedIndex = (selectedIndex + delta + activeShortlist.length) % activeShortlist.length;
        renderTft();
      }
    } else if (currentState === STATE_TARE_PROMPT) {
      // Rotate cancels tare and goes back to selection
      currentState = STATE_IDLE_SELECT;
      renderTft();
    }
  }

  function setupRotaryButton() {
    let pressTimer = null;
    let longPressTriggered = false;

    rotaryKnob.addEventListener('mousedown', () => {
      rotaryKnob.classList.add('pressed');
      longPressTriggered = false;
      pressTimer = setTimeout(() => {
        longPressTriggered = true;
        handleLongPress();
      }, 1500); // 1.5 second threshold
    });

    window.addEventListener('mouseup', () => {
      if (pressTimer) {
        clearTimeout(pressTimer);
        pressTimer = null;
      }
      if (rotaryKnob.classList.contains('pressed')) {
        rotaryKnob.classList.remove('pressed');
        if (!longPressTriggered) {
          handleShortClick();
        }
      }
    });

    btnCcw.addEventListener('click', () => rotateKnob(-1));
    btnCw.addEventListener('click', () => rotateKnob(1));
  }

  function handleShortClick() {
    switch (currentState) {
      case STATE_IDLE_SELECT:
        if (activeShortlist.length > 0) {
          currentState = STATE_TARE_PROMPT;
          renderTft();
        }
        break;

      case STATE_TARE_PROMPT:
        // Tare action
        tareOffset = rawScaleWeight;
        weightHistory = [];
        showToast("TARED!", `Scale zeroed to 0.0g`);
        logSerial(`[SCALE] Tared (zeroed) with container: ${tareOffset.toFixed(1)}g`);
        currentState = STATE_WEIGHING;
        renderTft();
        break;

      case STATE_WEIGHING:
        // Manual override if weight is stable
        let net = getNetWeight();
        if (net >= 1.0) {
          currentSettledWeight = net;
          currentState = STATE_RESULT;
          renderTft();
        }
        break;

      case STATE_RESULT:
        // Commit to running session total
        let food = activeShortlist[selectedIndex];
        let factor = currentSettledWeight / 100.0;
        let cals = factor * food.cal100;
        let prot = factor * food.protein100;
        let fat = factor * food.fat100;
        let carbs = factor * food.carb100;

        session.itemCount++;
        session.calories += cals;
        session.protein += prot;
        session.fat += fat;
        session.carbs += carbs;
        session.grams += currentSettledWeight;

        if (!session.items) session.items = [];
        session.items.unshift({
          name: food.name,
          grams: currentSettledWeight,
          calories: cals,
          protein: prot,
          fat: fat,
          carbs: carbs,
          time: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })
        });

        logSerial(`[SESSION] Added: ${food.name} (${currentSettledWeight.toFixed(1)}g) -> +${cals.toFixed(1)} kcal`);
        logSerial(`[SESSION] Running Total (${session.itemCount} items): ${session.calories.toFixed(1)} kcal | P:${session.protein.toFixed(1)}g | F:${session.fat.toFixed(1)}g | C:${session.carbs.toFixed(1)}g`);

        showToast("COMMITTED!", "Added to Session Total");
        updateSessionDisplay();
        currentState = STATE_IDLE_SELECT;
        renderTft();
        break;
    }
  }

  function handleLongPress() {
    // Reset running session totals
    session = { itemCount: 0, calories: 0, protein: 0, fat: 0, carbs: 0, grams: 0, items: [] };
    logSerial("[SESSION] Running session totals reset to 0.");
    showToast("SESSION RESET!", "Totals cleared to 0", true);
    updateSessionDisplay();
    currentState = STATE_IDLE_SELECT;
    renderTft();
  }

  // -------------------------------------------------------------
  // Live Session Display Synchronizer (Scale Screen + Under Phone)
  // -------------------------------------------------------------
  function updateSessionDisplay() {
    const titleText = `Session (${session.itemCount} items): ${Math.round(session.calories)} kcal`;
    const subText = `P:${session.protein.toFixed(1)}g  F:${session.fat.toFixed(1)}g  C:${session.carbs.toFixed(1)}g`;

    if (idleSessionTitle) idleSessionTitle.textContent = titleText;
    if (idleSessionSub) idleSessionSub.textContent = subText;

    if (underPhoneSessionTitle) underPhoneSessionTitle.textContent = titleText;
    if (underPhoneSessionSub) underPhoneSessionSub.textContent = subText;

    let totalMacroGrams = session.protein + session.fat + session.carbs;
    let pPct = totalMacroGrams > 0 ? Math.round((session.protein / totalMacroGrams) * 100) : 0;
    let fPct = totalMacroGrams > 0 ? Math.round((session.fat / totalMacroGrams) * 100) : 0;
    let cPct = totalMacroGrams > 0 ? Math.max(0, 100 - pPct - fPct) : 0;

    if (underPhonePPill) underPhonePPill.textContent = `P: ${session.protein.toFixed(1)}g (${pPct}%)`;
    if (underPhoneFPill) underPhoneFPill.textContent = `F: ${session.fat.toFixed(1)}g (${fPct}%)`;
    if (underPhoneCPill) underPhoneCPill.textContent = `C: ${session.carbs.toFixed(1)}g (${cPct}%)`;
    if (underPhoneWtPill) underPhoneWtPill.textContent = `⚖ ${session.grams.toFixed(1)}g`;

    if (underPhoneHistoryCount) {
      underPhoneHistoryCount.textContent = (session.items ? session.items.length : session.itemCount);
    }

    if (underPhoneSessionItems) {
      if (!session.items || session.items.length === 0) {
        underPhoneSessionItems.innerHTML = `<div class="session-empty-hint">Weigh items on the scale and click to commit them to this session total.</div>`;
      } else {
        underPhoneSessionItems.innerHTML = session.items.map((it, idx) => `
          <div class="session-item-row">
            <div class="session-item-name">
              <span class="session-item-idx">#${idx + 1}</span>
              <strong>${it.name}</strong>
              <span class="session-item-wt">(${it.grams.toFixed(1)}g)</span>
            </div>
            <div class="session-item-macros">
              <span class="session-item-cal">${Math.round(it.calories)} kcal</span>
              <span class="session-item-pfc">P:${it.protein.toFixed(1)}g F:${it.fat.toFixed(1)}g C:${it.carbs.toFixed(1)}g</span>
            </div>
          </div>
        `).join('');
      }
    }
  }

  // -------------------------------------------------------------
  // Serial Monitor Console & Calibration Wizard
  // -------------------------------------------------------------
  function logSerial(msg) {
    let ts = new Date().toLocaleTimeString();
    serialOutput.textContent += `[${ts}] ${msg}\n`;
    serialOutput.scrollTop = serialOutput.scrollHeight;
  }

  function handleSerialInput() {
    let input = serialInput.value.trim();
    if (!input && calWizardStep !== 1) return;
    serialInput.value = '';

    logSerial(`> ${input}`);

    if (calWizardStep === 0) {
      if (input.toUpperCase() === "CAL" || input.toUpperCase() === "C") {
        calWizardStep = 1;
        logSerial("\n==================================================");
        logSerial("         SMART SCALE CALIBRATION WIZARD           ");
        logSerial("==================================================");
        logSerial("1. REMOVE all weight from the scale platform.");
        logSerial("   Press ENTER or send any key when clear...");
      } else {
        logSerial("Unknown serial command. Type 'CAL' to calibrate scale.");
      }
    } else if (calWizardStep === 1) {
      // Step 1: Zero offset
      logSerial("Zeroing scale...");
      tareOffset = 0.0;
      logSerial("Zero offset recorded.");
      calWizardStep = 2;
      logSerial("\n2. Place a KNOWN reference weight on the scale platform.");
      logSerial("   (e.g., a 100g, 200g, or 500g calibrated weight)");
      logSerial("   Type the weight in GRAMS (e.g. 200.0) and press ENTER:");
    } else if (calWizardStep === 2) {
      let knownGrams = parseFloat(input);
      if (isNaN(knownGrams) || knownGrams <= 0) {
        logSerial("ERROR: Invalid weight entered. Calibration cancelled.");
        calWizardStep = 0;
      } else {
        logSerial("Sampling weight...");
        let raw = rawScaleWeight > 0 ? rawScaleWeight : knownGrams;
        calFactor = (raw / knownGrams) * -2180.0;
        logSerial(`Raw reading: ${Math.round(raw * 2180)}, Known grams: ${knownGrams.toFixed(2)}`);
        logSerial(`Calculated 1kg Calibration Factor: ${calFactor.toFixed(4)}`);
        logSerial("SUCCESS! New calibration factor saved to flash NVS.");
        logSerial("Scale is now ready for accurate weighing.");
        logSerial("==================================================\n");
        calWizardStep = 0;
      }
    }
  }

  // -------------------------------------------------------------
  // Android Companion App Logic (Room DB, Shortlist, Wi-Fi Sync)
  // -------------------------------------------------------------
  let allFoods = [];

  async function loadFoodsData() {
    try {
      if (window.IFCT_FOODS_DATA && Array.isArray(window.IFCT_FOODS_DATA) && window.IFCT_FOODS_DATA.length > 0) {
        allFoods = window.IFCT_FOODS_DATA;
      } else {
        const res = await fetch('foods.json');
        allFoods = await res.json();
      }
      dbCountLabel.textContent = `${allFoods.length} Raw Foods (IFCT 2017) • Tap ★ to shortlist`;
      renderPhoneFoodList(allFoods);
      renderPhoneShortlist();
    } catch (e) {
      console.warn("fetch('foods.json') failed, falling back to embedded dataset:", e);
      if (window.IFCT_FOODS_DATA && Array.isArray(window.IFCT_FOODS_DATA)) {
        allFoods = window.IFCT_FOODS_DATA;
        dbCountLabel.textContent = `${allFoods.length} Raw Foods (IFCT 2017) • Tap ★ to shortlist`;
        renderPhoneFoodList(allFoods);
        renderPhoneShortlist();
      }
    }
  }

  function renderPhoneFoodList(items) {
    phoneFoodList.innerHTML = '';
    let displayItems = items.slice(0, 100); // Efficient rendering of first 100

    displayItems.forEach(food => {
      let isShortlisted = phoneShortlist.some(s => s.name === food.name);

      let card = document.createElement('div');
      card.className = 'phone-food-card';
      card.innerHTML = `
        <div class="food-card-left">
          <div class="card-food-name">${food.name}</div>
          <div class="card-food-group">${food.group || 'General'}</div>
          <div class="card-macro-breakdown">Per 100g: ${Math.round(food.cal100)} kcal | P: ${food.protein100}g | F: ${food.fat100}g | C: ${food.carb100}g</div>
        </div>
        <button class="star-toggle-btn ${isShortlisted ? 'active' : ''}" title="Toggle Shortlist">
          ${isShortlisted ? '★' : '☆'}
        </button>
      `;

      card.querySelector('.star-toggle-btn').addEventListener('click', (e) => {
        e.stopPropagation();
        togglePhoneShortlist(food);
      });

      phoneFoodList.appendChild(card);
    });
  }

  function togglePhoneShortlist(food) {
    let idx = phoneShortlist.findIndex(s => s.name === food.name);
    let newlyAdded = false;
    if (idx !== -1) {
      // Remove
      phoneShortlist.splice(idx, 1);
    } else {
      // Non-negotiable rule: Strictly cap at 20 items
      if (phoneShortlist.length >= 20) {
        alert("⚠️ Maximum 20 items allowed in shortlist!\n\nRemove an item before adding another.");
        return;
      }
      newlyAdded = true;
      phoneShortlist.push({
        slot: phoneShortlist.length,
        name: food.name,
        cal100: food.cal100,
        protein100: food.protein100,
        fat100: food.fat100,
        carb100: food.carb100
      });
    }

    // Re-index slots
    phoneShortlist.forEach((item, i) => item.slot = i);

    renderPhoneShortlist();
    renderPhoneFoodList(filterFoods(phoneSearchInput.value));

    // Live-sync scale hardware shortlist and update display
    activeShortlist = [...phoneShortlist];
    if (newlyAdded) {
      selectedIndex = activeShortlist.length - 1;
      logSerial(`[SHORTLIST] Phone starred "${food.name}". Synced to Scale Slot ${String(selectedIndex).padStart(2, '0')}.`);
      showToast("SCALE UPDATED", `Slot ${selectedIndex + 1}/${activeShortlist.length}: ${food.name}`);
    } else {
      if (selectedIndex >= activeShortlist.length) {
        selectedIndex = Math.max(0, activeShortlist.length - 1);
      }
      logSerial(`[SHORTLIST] Removed "${food.name}". Scale now has ${activeShortlist.length} items.`);
      showToast("SCALE SYNCED", `${activeShortlist.length} Items Remaining`);
    }
    renderTft();
  }

  function filterFoods(query) {
    if (!query) return allFoods;
    let q = query.toLowerCase();
    return allFoods.filter(f => f.name.toLowerCase().includes(q) || (f.group && f.group.toLowerCase().includes(q)));
  }

  function renderPhoneShortlist() {
    phoneCounterPill.textContent = `${phoneShortlist.length} / 20`;
    capacityNumberLabel.textContent = `${phoneShortlist.length} / 20 Slots Filled`;
    capacityBarFill.style.width = `${(phoneShortlist.length / 20) * 100}%`;
    syncDescText.textContent = `Ready to sync active shortlist (${phoneShortlist.length} items) to device.`;

    phoneShortlistItems.innerHTML = '';
    if (phoneShortlist.length === 0) {
      phoneShortlistItems.innerHTML = '<div style="text-align:center; padding:20px; color:#64748B;">Shortlist is empty. Tap star on any food in database.</div>';
      return;
    }

    phoneShortlist.forEach((item, index) => {
      let card = document.createElement('div');
      card.className = 'shortlist-item-card';
      card.innerHTML = `
        <div>
          <span class="slot-tag">SLOT ${String(index).padStart(2, '0')}</span>
          <div class="card-food-name">${item.name}</div>
          <div class="card-macro-breakdown">Per 100g: ${Math.round(item.cal100)} kcal | P: ${item.protein100}g | F: ${item.fat100}g | C: ${item.carb100}g</div>
        </div>
        <button class="btn-remove-item" title="Remove from shortlist">🗑️</button>
      `;

      card.querySelector('.btn-remove-item').addEventListener('click', () => {
        phoneShortlist.splice(index, 1);
        phoneShortlist.forEach((it, i) => it.slot = i);
        renderPhoneShortlist();
        renderPhoneFoodList(filterFoods(phoneSearchInput.value));
        activeShortlist = [...phoneShortlist];
        if (selectedIndex >= activeShortlist.length) {
          selectedIndex = Math.max(0, activeShortlist.length - 1);
        }
        renderTft();
        showToast("SCALE SYNCED", `${activeShortlist.length} Items Remaining`);
      });

      phoneShortlistItems.appendChild(card);
    });
  }

  // -------------------------------------------------------------
  // Wi-Fi HTTP REST Synchronization Pipeline
  // -------------------------------------------------------------
  function appendProtocolLog(msg) {
    protocolLog.textContent += `\n${msg}`;
    protocolLog.scrollTop = protocolLog.scrollHeight;
  }

  function testWifiConnection() {
    let host = (scaleHostInput ? scaleHostInput.value.trim() : "") || "smartscale.local";
    appendProtocolLog(`\n[HTTP] GET http://${host}/api/status`);
    if (btnTestConn) {
      btnTestConn.disabled = true;
      btnTestConn.textContent = "Connecting to Scale...";
    }

    setTimeout(() => {
      let statusJson = JSON.stringify({
        status: "ONLINE",
        ip: "192.168.1.150",
        mdns: "smartscale.local",
        items: activeShortlist.length,
        cal_factor: calFactor,
        uptime_s: 128
      });

      appendProtocolLog(`<- HTTP 200 OK: ${statusJson}`);
      appendProtocolLog(`✓ Scale found and responding via Wi-Fi REST API!`);
      logSerial(`[HTTP] GET /api/status - 200 OK (Remote client connected)`);

      syncConnStatus.textContent = `Online (${host})`;
      syncConnStatus.classList.add('connected');
      syncStatusIcon.classList.add('connected');
      if (syncDescText) {
        syncDescText.textContent = `Ready to sync active shortlist (${phoneShortlist.length} items) to ${host}.`;
      }

      if (btnTestConn) {
        btnTestConn.disabled = false;
        btnTestConn.textContent = "Scale Responding (GET /api/status)";
      }
    }, 400);
  }

  function executeWifiSync() {
    if (phoneShortlist.length === 0) {
      alert("Shortlist is empty! Add items from the database tab first.");
      return;
    }

    let host = (scaleHostInput ? scaleHostInput.value.trim() : "") || "smartscale.local";
    btnPhoneSyncNow.disabled = true;
    syncProgressBar.classList.remove('hidden');
    syncProgressFill.style.width = '0%';

    let payload = {
      items: phoneShortlist.map((item, idx) => ({
        slot: idx,
        name: item.name,
        cal100: item.cal100,
        protein100: item.protein100,
        fat100: item.fat100,
        carb100: item.carb100
      }))
    };

    let jsonStr = JSON.stringify(payload);
    let byteSize = new Blob([jsonStr]).size;

    appendProtocolLog(`\n=== STARTING WI-FI REST SHORTLIST SYNC ===`);
    appendProtocolLog(`-> POST http://${host}/api/sync`);
    appendProtocolLog(`   Content-Type: application/json (${byteSize} bytes, ${phoneShortlist.length} foods)`);

    logSerial(`[HTTP] POST /api/sync received (${payload.items.length} items, ${byteSize} bytes)`);
    logSerial(`[SHORTLIST] Clearing previous NVS shortlist...`);

    let progress = 0;
    let progressTimer = setInterval(() => {
      progress += 25;
      syncProgressFill.style.width = `${progress}%`;

      if (progress >= 100) {
        clearInterval(progressTimer);

        payload.items.forEach(item => {
          logSerial(`[SHORTLIST] Slot ${item.slot} set: ${item.name} (${item.cal100} kcal/100g)`);
        });

        // Commit to active scale shortlist
        activeShortlist = [...phoneShortlist];
        selectedIndex = 0;
        logSerial(`[SHORTLIST] ${activeShortlist.length} items committed to NVS flash storage.`);
        logSerial(`[HTTP] Responding 200 OK: {"status":"OK","saved":${activeShortlist.length}}`);

        appendProtocolLog(`<- HTTP 200 OK: {"status":"OK","saved":${activeShortlist.length}}`);
        appendProtocolLog(`✓ SYNC SUCCESS: Scale NVS updated over local Wi-Fi!`);

        syncProgressBar.classList.add('hidden');
        btnPhoneSyncNow.disabled = false;
        if (syncDescText) {
          syncDescText.textContent = `Synced ${activeShortlist.length} items to scale over Wi-Fi.`;
        }

        showToast("SYNC COMPLETE", `Wi-Fi: ${activeShortlist.length} Items Saved`);
        renderTft();
      }
    }, 150);
  }

  // -------------------------------------------------------------
  // Event Listeners Setup
  // -------------------------------------------------------------
  function setupEventListeners() {
    // Weight presets for 1kg scale
    btnPlaceBowl.addEventListener('click', () => setScaleWeight(65.0, "Prep Bowl"));
    btnAddOil.addEventListener('click', () => setScaleWeight(rawScaleWeight + 15.0, "Bowl + Mustard Oil"));
    btnAddPaneer.addEventListener('click', () => setScaleWeight(rawScaleWeight + 80.0, "Bowl + Paneer"));
    btnAddRice.addEventListener('click', () => setScaleWeight(rawScaleWeight + 150.0, "Bowl + Raw Rice"));
    btnOverload.addEventListener('click', () => setScaleWeight(1050.0, "Excess Weight"));
    btnRemoveAll.addEventListener('click', () => {
      tareOffset = 0.0;
      setScaleWeight(0.0, "Scale Platform Empty");
    });

    // Slider
    weightSlider.addEventListener('input', (e) => {
      setScaleWeight(parseFloat(e.target.value));
    });

    // Serial console
    btnSendSerial.addEventListener('click', handleSerialInput);
    serialInput.addEventListener('keydown', (e) => {
      if (e.key === 'Enter') handleSerialInput();
    });
    btnClearSerial.addEventListener('click', () => {
      serialOutput.textContent = '';
    });

    // Phone search
    phoneSearchInput.addEventListener('input', (e) => {
      renderPhoneFoodList(filterFoods(e.target.value));
    });

    // Phone clear all
    phoneBtnClearAll.addEventListener('click', () => {
      if (confirm("Remove all items from your scale shortlist?")) {
        phoneShortlist = [];
        renderPhoneShortlist();
        renderPhoneFoodList(filterFoods(phoneSearchInput.value));
        activeShortlist = [];
        selectedIndex = 0;
        renderTft();
        showToast("SHORTLIST CLEARED", "All scale items removed", true);
        logSerial(`[SHORTLIST] Scale shortlist cleared to 0 items.`);
      }
    });

    // Custom Food Modal & Brand Templates
    if (btnOpenCustomModal) {
      btnOpenCustomModal.addEventListener('click', () => {
        customFoodModal.classList.remove('hidden');
      });
    }

    if (btnCloseModal) {
      btnCloseModal.addEventListener('click', () => {
        if (customFoodModal) customFoodModal.classList.add('hidden');
      });
    }

    if (customFoodModal) {
      customFoodModal.addEventListener('click', (e) => {
        if (e.target === customFoodModal) {
          customFoodModal.classList.add('hidden');
        }
      });
    }

    // Quick Brand Chips
    document.querySelectorAll('.brand-chip').forEach(chip => {
      chip.addEventListener('click', () => {
        if (customBrandInput) customBrandInput.value = chip.dataset.brand || "";
        if (customNameInput) customNameInput.value = chip.dataset.name || "";
        if (customGroupInput) customGroupInput.value = chip.dataset.group || "Milk and Milk Products";
        if (customCalInput) customCalInput.value = chip.dataset.cal || "0";
        if (customProtInput) customProtInput.value = chip.dataset.prot || "0";
        if (customFatInput) customFatInput.value = chip.dataset.fat || "0";
        if (customCarbInput) customCarbInput.value = chip.dataset.carb || "0";
      });
    });

    if (btnSaveCustomFood) {
      btnSaveCustomFood.addEventListener('click', (e) => {
        if (e) e.preventDefault();
        try {
          let brand = (customBrandInput ? customBrandInput.value.trim() : "");
          let name = (customNameInput ? customNameInput.value.trim() : "");
          let group = (customGroupInput ? customGroupInput.value.trim() : "") || "Milk and Milk Products";
          let cal = parseFloat(customCalInput ? customCalInput.value : "0") || 0;
          let prot = parseFloat(customProtInput ? customProtInput.value : "0") || 0;
          let fat = parseFloat(customFatInput ? customFatInput.value : "0") || 0;
          let carb = parseFloat(customCarbInput ? customCarbInput.value : "0") || 0;

          if (!name) {
            alert("Please enter a name for your custom / brand food!");
            return;
          }

          // Format nice full name (e.g. "Amul Malai Paneer", "Pintola Peanut Butter")
          let fullName = brand && !name.toLowerCase().includes(brand.toLowerCase())
            ? `${brand} ${name}`
            : name;

          let newFood = {
            id: Date.now(),
            code: "BRAND",
            name: fullName,
            group: group,
            cal100: cal,
            protein100: prot,
            fat100: fat,
            carb100: carb,
            isShortlisted: 0
          };

          // Add to database
          allFoods.unshift(newFood);
          if (dbCountLabel) {
            dbCountLabel.textContent = `${allFoods.length} Foods (IFCT + Custom Brands) • Tap ★ to shortlist`;
          }

          // Check if user requested auto-shortlisting
          if (customShortlistCheck && customShortlistCheck.checked) {
            if (phoneShortlist.length < 20) {
              newFood.isShortlisted = 1;
              newFood.slot = 0;
              phoneShortlist.unshift(newFood);
              phoneShortlist.forEach((item, idx) => item.slot = idx);
              renderPhoneShortlist();

              // Immediately sync and display on scale hardware
              activeShortlist = [...phoneShortlist];
              selectedIndex = 0; // Focus on the newly added food
              renderTft();
              logSerial(`[SHORTLIST] Added Brand Food "${fullName}" directly to Scale Slot 00.`);
              showToast("SCALE UPDATED", `Slot 1/${activeShortlist.length}: ${fullName}`);
            } else {
              alert("Food saved, but shortlist is full (20/20 max). Remove an item to add it.");
            }
          }

          if (customFoodModal) {
            customFoodModal.classList.add('hidden');
          }
          let query = (phoneSearchInput && phoneSearchInput.value) ? phoneSearchInput.value : "";
          renderPhoneFoodList(filterFoods(query));
        } catch (err) {
          console.error("Error saving custom food:", err);
          alert("Error saving custom food: " + err.message);
        }
      });
    }

    // Phone Wi-Fi controls
    if (btnTestConn) btnTestConn.addEventListener('click', testWifiConnection);
    if (btnPhoneSyncNow) btnPhoneSyncNow.addEventListener('click', executeWifiSync);

    // Bottom Navigation
    navTabs.forEach(tab => {
      tab.addEventListener('click', () => {
        navTabs.forEach(t => t.classList.remove('active'));
        tab.classList.add('active');

        let target = tab.dataset.tab;
        tabDatabase.classList.toggle('hidden', target !== 'tab-database');
        tabShortlist.classList.toggle('hidden', target !== 'tab-shortlist');
        tabSync.classList.toggle('hidden', target !== 'tab-sync');

        if (target === 'tab-database') appBarTitle.textContent = "Food Database";
        if (target === 'tab-shortlist') appBarTitle.textContent = "Scale Shortlist";
        if (target === 'tab-sync') {
          appBarTitle.textContent = "Wi-Fi Scale Sync";
          if (syncDescText) {
            syncDescText.textContent = `Ready to sync active shortlist (${phoneShortlist.length} items) to device.`;
          }
        }
      });
    });

    // Reset Session Total from Under-Phone Widget
    if (underPhoneBtnReset) {
      underPhoneBtnReset.addEventListener('click', () => {
        session = { itemCount: 0, calories: 0, protein: 0, fat: 0, carbs: 0, grams: 0, items: [] };
        logSerial("[SESSION] Running session totals reset from companion panel.");
        showToast("SESSION RESET!", "Totals cleared to 0", true);
        updateSessionDisplay();
        renderTft();
      });
    }
  }

  // Run boot on load
  window.addEventListener('DOMContentLoaded', boot);
})();
