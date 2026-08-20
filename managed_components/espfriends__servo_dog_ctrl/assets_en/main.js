/*
SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
SPDX-License-Identifier: Apache-2.0
*/

const ACTION_NAMES = {
  '1': 'Lie', '2': 'Bow', '3': 'Lean', '4': 'Wiggle',
  '5': 'Rock', '6': 'Sway', '7': 'Shake', '8': 'Poke',
  '9': 'Kick', '10': 'Jump →', '11': '← Jump', '12': 'Retract',
  'F': 'Forward', 'B': 'Backward', 'L': 'Turn Left', 'R': 'Turn Right'
};

const DISTANCE_THRESHOLD = 0.2;
const SEND_INTERVAL = 500;

async function sendRequest(endpoint, data = null) {
  try {
    const options = {
      method: data ? 'POST' : 'GET',
      headers: { 'Content-Type': 'application/json' }
    };
    
    if (data) {
      options.body = JSON.stringify(data);
    }

    const response = await fetch(endpoint, options);

    if (!response.ok) {
      const errorData = await response.json();
      if (errorData.error === "Control disabled in calibration mode") {
        throw new Error('Control functions unavailable in calibration mode');
      }
      throw new Error('Connection error');
    }

    return await response.json();
  } catch (error) {
    console.error(error);
    throw error;
  }
}

class ControlPanel {
  constructor() {
    this.controlError = document.getElementById('control-error');
    this.actionButtons = document.getElementById('action-buttons');
    this.joystickZone = document.getElementById('joystick');
    this.lastDir = null;
    this.interval = null;
    
    this.initActionButtons();
    this.initJoystick();
  }

  initActionButtons() {
    const buttonContainer = document.createElement('div');
    buttonContainer.className = 'button-container';
    
    for (let row = 0; row < 4; row++) {
      const buttonRow = document.createElement('div');
      buttonRow.className = 'button-row';
      for (let i = 1; i <= 3; i++) {
        const buttonNumber = row * 3 + i;
        if (buttonNumber > 12) break;
        const btn = document.createElement('button');
        btn.textContent = ACTION_NAMES[buttonNumber.toString()];
        btn.onclick = () => this.sendAction(buttonNumber);
        buttonRow.appendChild(btn);
      }
      buttonContainer.appendChild(buttonRow);
    }
    
    this.actionButtons.appendChild(buttonContainer);
  }

  async sendAction(id) {
    try {
      await sendRequest('/control', { action: id.toString() });
      this.controlError.textContent = '';
    } catch (error) {
      this.controlError.textContent = error.message;
    }
  }

  initJoystick() {
    const manager = nipplejs.create({ 
      zone: this.joystickZone, 
      size: 120,
      mode: 'dynamic',
      position: { left: '50%', top: '50%' },
      color: '#00BFFF',
    });

    manager.on('start move end', (evt, data) => this.handleJoystickEvent(evt, data));
  }

  handleJoystickEvent(evt, data) {
    if (evt.type === 'end') {
      clearInterval(this.interval);
      this.lastDir = null;
      return;
    }

    if (data.force < DISTANCE_THRESHOLD) {
      clearInterval(this.interval);
      this.lastDir = null;
      return;
    }

    const angle = data.angle && data.angle.radian;
    if (angle === undefined) return;

    const degrees = (angle * 180 / Math.PI + 360) % 360;
    
    let dir = null;
    if (degrees >= 45 && degrees < 135) dir = 'F';
    else if (degrees >= 135 && degrees < 225) dir = 'L';
    else if (degrees >= 225 && degrees < 315) dir = 'B';
    else dir = 'R';

    if (dir && dir !== this.lastDir) {
      this.lastDir = dir;
      clearInterval(this.interval);
      this.sendMove(dir);
      this.interval = setInterval(() => this.sendMove(dir), SEND_INTERVAL);
    }
  }

  async sendMove(dir) {
    try {
      await sendRequest('/control', { move: dir });
      this.controlError.textContent = '';
    } catch (error) {
      this.controlError.textContent = error.message;
    }
  }
}

class CalibrationPanel {
  constructor() {
    this.configError = document.getElementById('config-error');
    this.cfg = { fl: 0, fr: 0, bl: 0, br: 0 };
    this.current = null;
    this.display = document.getElementById('value-display');
    this.minus = document.getElementById('minus');
    this.plus = document.getElementById('plus');
    this.calibrationIntro = document.getElementById('calibration-intro');
    this.calibrationInterface = document.getElementById('calibration-interface');
    this.startCalibrationBtn = document.getElementById('start-calibration');
    this.exitCalibrationBtn = document.getElementById('exit-calibration');

    this.initEventListeners();
  }

  initEventListeners() {
    this.startCalibrationBtn.onclick = () => this.startCalibration();
    this.exitCalibrationBtn.onclick = () => this.exitCalibration();
    this.minus.onclick = () => this.adjust(-1);
    this.plus.onclick = () => this.adjust(1);

    document.querySelectorAll('.servo').forEach(s => {
      s.onclick = () => this.selectServo(s);
    });

    document.querySelector('.vehicle').addEventListener('click', (e) => {
      if (e.target.classList.contains('vehicle')) {
        this.deselectServo();
      }
    });
  }

  async startCalibration() {
    if (location.hostname === 'localhost') {
      this.cfg = { fl: 0, fr: 0, bl: 0, br: 0 };
      this.updateServoDisplay();
      this.calibrationIntro.style.display = 'none';
      this.calibrationInterface.style.display = 'block';
      return;
    }

    try {
      const data = await sendRequest('/start_calibration');
      this.cfg = data;
      this.updateServoDisplay();
      this.calibrationIntro.style.display = 'none';
      this.calibrationInterface.style.display = 'block';
    } catch (error) {
      this.configError.textContent = error.message;
    }
  }

  async exitCalibration() {
    try {
      await sendRequest('/exit_calibration');
      this.calibrationInterface.style.display = 'none';
      this.calibrationIntro.style.display = 'block';
    } catch (error) {
      this.configError.textContent = error.message;
    }
  }

  selectServo(servo) {
    document.querySelectorAll('.servo').forEach(x => x.classList.remove('active'));
    servo.classList.add('active');
    this.current = servo.dataset.pos;
    this.display.textContent = this.cfg[this.current];
    document.querySelector('.adjust').style.display = 'flex';
  }

  deselectServo() {
    document.querySelectorAll('.servo').forEach(x => x.classList.remove('active'));
    this.current = null;
    document.querySelector('.adjust').style.display = 'none';
  }

  async adjust(delta) {
    if (!this.current) return;
    
    const newValue = this.cfg[this.current] + delta;
    if (newValue >= -25 && newValue <= 25) {
      this.cfg[this.current] = newValue;
    } else if (newValue < -25) {
      this.cfg[this.current] = -25;
    } else if (newValue > 25) {
      this.cfg[this.current] = 25;
    }

    this.updateServoDisplay();
    try {
      await sendRequest('/adjust', {
        servo: this.current,
        value: this.cfg[this.current]
      });
    } catch (error) {
      this.configError.textContent = error.message;
    }
  }

  updateServoDisplay() {
    Object.entries(this.cfg).forEach(([pos, value]) => {
      document.querySelector(`.servo[data-pos='${pos}'] .value`).textContent = value;
    });
    this.display.textContent = this.cfg[this.current];
  }
}

class CustomActionPanel {
  constructor() {
    this.customError = document.getElementById('custom-error');
    this.actionSelect = document.getElementById('action-select');
    this.delayInput = document.getElementById('delay-input');
    this.addActionBtn = document.getElementById('add-action');
    this.sequenceList = document.querySelector('.sequence-list');
    this.saveSequenceBtn = document.getElementById('save-sequence');
    this.playSequenceBtn = document.getElementById('play-sequence');

    this.sequence = [];
    this.isPlaying = false;
    this.currentTimeout = null;

    this.initEventListeners();
    this.loadSavedSequence();
  }

  initEventListeners() {
    this.addActionBtn.onclick = () => this.addAction();
    this.sequenceList.addEventListener('click', (e) => {
      if (e.target.classList.contains('remove')) {
        const index = parseInt(e.target.dataset.index);
        this.sequence.splice(index, 1);
        this.updateSequenceDisplay();
      }
    });
    this.saveSequenceBtn.onclick = () => this.saveSequence();
    this.playSequenceBtn.onclick = () => this.playSequence();
  }

  addAction() {
    const action = this.actionSelect.value;
    const delay = parseFloat(this.delayInput.value);

    if (!action) {
      this.customError.textContent = 'Please select an action';
      return;
    }

    if (isNaN(delay) || delay < 0 || delay > 10) {
      this.customError.textContent = 'Delay time must be between 0-10 seconds';
      return;
    }

    if (this.sequence.length >= 4) {
      this.customError.textContent = 'Maximum of 4 actions allowed';
      return;
    }

    this.sequence.push({ action, delay });
    this.updateSequenceDisplay();
    this.customError.textContent = '';
  }

  updateSequenceDisplay() {
    this.sequenceList.innerHTML = '';
    this.sequence.forEach((item, index) => {
      const div = document.createElement('div');
      div.className = 'sequence-item';
      div.innerHTML = `
        <span>${ACTION_NAMES[item.action]}</span>
        <span>then delay: ${item.delay}s</span>
        <span class="remove" data-index="${index}">×</span>
      `;
      this.sequenceList.appendChild(div);
    });

    const hasItems = this.sequence.length > 0;
    this.saveSequenceBtn.disabled = !hasItems;
    this.playSequenceBtn.disabled = !hasItems || this.isPlaying;
  }

  async playSequence() {
    if (this.isPlaying) return;
    this.isPlaying = true;
    this.updateSequenceDisplay();

    for (const item of this.sequence) {
      if (!this.isPlaying) break;

      try {
        await sendRequest('/control', 
          ['F', 'B', 'L', 'R'].includes(item.action) ? 
          { move: item.action } : 
          { action: item.action }
        );

        await new Promise(resolve => {
          this.currentTimeout = setTimeout(resolve, item.delay * 1000);
        });
      } catch (error) {
        this.customError.textContent = error.message;
        break;
      }
    }

    this.isPlaying = false;
    this.currentTimeout = null;
    this.updateSequenceDisplay();
  }

  saveSequence() {
    try {
      localStorage.setItem('savedSequence', JSON.stringify(this.sequence));
      this.customError.textContent = 'Actions saved';
    } catch (error) {
      this.customError.textContent = 'Save failed';
    }
  }

  loadSavedSequence() {
    try {
      const saved = localStorage.getItem('savedSequence');
      if (saved) {
        this.sequence = JSON.parse(saved);
        this.updateSequenceDisplay();
      }
    } catch (error) {
      console.error('Failed to load saved actions:', error);
    }
  }
}

document.addEventListener('DOMContentLoaded', () => {
  // disable double click zoom
  document.addEventListener('dblclick', (e) => e.preventDefault(), { passive: false });

  document.querySelectorAll('.tab').forEach(tab => {
    tab.onclick = () => {
      document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
      tab.classList.add('active');
      document.querySelectorAll('.panel').forEach(p => p.style.display = 'none');
      document.getElementById(tab.dataset.tab).style.display = 'block';
    };
  });

  new ControlPanel();
  new CalibrationPanel();
  new CustomActionPanel();
}); 