(() => {
  "use strict";

  const presentation = document.getElementById("presentation");
  const fullscreenRoot = document.getElementById("fullscreenRoot");
  const slides = Array.from(document.querySelectorAll(".slide"));
  const prevButton = document.getElementById("prevButton");
  const nextButton = document.getElementById("nextButton");
  const fullscreenButton = document.getElementById("fullscreenButton");
  const listButton = document.getElementById("listButton");
  const slideDialog = document.getElementById("slideDialog");
  const dialogCloseButton = document.getElementById("dialogCloseButton");
  const slideList = document.getElementById("slideList");
  const currentNumber = document.getElementById("currentNumber");
  const totalNumber = document.getElementById("totalNumber");
  const progressBar = document.getElementById("progressBar");
  const jumpStatus = document.getElementById("jumpStatus");
  const sectionButtons = Array.from(document.querySelectorAll("[data-section-target]"));

  if (!presentation || slides.length === 0) {
    return;
  }

  let currentIndex = 0;
  let numberBuffer = "";
  let numberTimer = 0;
  let touchStartX = null;
  let touchStartY = null;

  const formatNumber = (value) => String(value).padStart(2, "0");
  const clampIndex = (value) => Math.max(0, Math.min(slides.length - 1, value));

  function getIndexFromHash() {
    const match = window.location.hash.match(/^#slide-(\d+)$/);
    if (!match) return 0;
    return clampIndex(Number(match[1]) - 1);
  }

  function setHash(index) {
    const nextHash = `#slide-${index + 1}`;
    if (window.location.hash === nextHash) return;

    try {
      window.history.replaceState(null, "", nextHash);
    } catch (_error) {
      window.location.hash = nextHash;
    }
  }

  function updateSection(sectionName) {
    sectionButtons.forEach((button) => {
      const isActive = button.dataset.sectionTarget === sectionName;
      button.classList.toggle("is-active", isActive);
      button.setAttribute("aria-current", isActive ? "step" : "false");
    });
  }

  function updateSlideList() {
    slideList.querySelectorAll("button").forEach((button, index) => {
      const isCurrent = index === currentIndex;
      button.classList.toggle("is-current", isCurrent);
      button.setAttribute("aria-current", isCurrent ? "page" : "false");
    });
  }

  function goToSlide(index, options = {}) {
    const nextIndex = clampIndex(index);
    const previousIndex = currentIndex;
    const direction = options.direction || (nextIndex < previousIndex ? "prev" : "next");

    currentIndex = nextIndex;
    presentation.dataset.direction = direction;

    slides.forEach((slide, slideIndex) => {
      const isActive = slideIndex === currentIndex;
      slide.classList.toggle("is-active", isActive);
      slide.setAttribute("aria-hidden", String(!isActive));
    });

    const activeSlide = slides[currentIndex];
    currentNumber.textContent = formatNumber(currentIndex + 1);
    totalNumber.textContent = formatNumber(slides.length);
    progressBar.style.width = `${((currentIndex + 1) / slides.length) * 100}%`;
    prevButton.disabled = currentIndex === 0;
    nextButton.disabled = currentIndex === slides.length - 1;
    updateSection(activeSlide.dataset.section || "");
    updateSlideList();

    document.title = `${formatNumber(currentIndex + 1)} · ${activeSlide.dataset.title} — Parcel Knight`;
    if (options.updateHash !== false) setHash(currentIndex);
  }

  function moveSlide(delta) {
    goToSlide(currentIndex + delta, { direction: delta < 0 ? "prev" : "next" });
  }

  function createSlideList() {
    const fragment = document.createDocumentFragment();

    slides.forEach((slide, index) => {
      const item = document.createElement("li");
      const button = document.createElement("button");
      const number = document.createElement("b");
      const title = document.createElement("span");
      const section = document.createElement("small");

      button.type = "button";
      button.dataset.slideIndex = String(index);
      button.setAttribute("aria-label", `${index + 1}번 슬라이드 ${slide.dataset.title}로 이동`);
      number.textContent = formatNumber(index + 1);
      title.textContent = slide.dataset.title || `슬라이드 ${index + 1}`;
      section.textContent = slide.dataset.section || "";

      button.append(number, title, section);
      button.addEventListener("click", () => {
        goToSlide(index);
        slideDialog.close();
      });
      item.append(button);
      fragment.append(item);
    });

    slideList.replaceChildren(fragment);
  }

  function openSlideList() {
    if (typeof slideDialog.showModal === "function") {
      slideDialog.showModal();
      const currentButton = slideList.querySelector("button.is-current");
      currentButton?.focus();
    }
  }

  function commitNumberJump() {
    const slideNumber = Number(numberBuffer);
    numberBuffer = "";
    jumpStatus.textContent = "";

    if (slideNumber >= 1 && slideNumber <= slides.length) {
      goToSlide(slideNumber - 1);
    }
  }

  function queueNumberJump(digit) {
    window.clearTimeout(numberTimer);

    if (numberBuffer === "" && digit === "0") return;
    const candidate = `${numberBuffer}${digit}`;
    if (Number(candidate) > slides.length) {
      numberBuffer = digit === "0" ? "" : digit;
    } else {
      numberBuffer = candidate;
    }

    jumpStatus.textContent = numberBuffer ? `${numberBuffer}번으로 이동…` : "";
    numberTimer = window.setTimeout(commitNumberJump, 650);
  }

  async function toggleFullscreen() {
    try {
      if (document.fullscreenElement) {
        await document.exitFullscreen();
      } else {
        await fullscreenRoot.requestFullscreen();
      }
    } catch (_error) {
      jumpStatus.textContent = "전체 화면을 사용할 수 없습니다.";
      window.setTimeout(() => { jumpStatus.textContent = ""; }, 1600);
    }
  }

  prevButton.addEventListener("click", () => moveSlide(-1));
  nextButton.addEventListener("click", () => moveSlide(1));
  fullscreenButton.addEventListener("click", toggleFullscreen);
  listButton.addEventListener("click", openSlideList);
  dialogCloseButton.addEventListener("click", () => slideDialog.close());

  sectionButtons.forEach((button) => {
    button.addEventListener("click", () => {
      const targetIndex = slides.findIndex((slide) => slide.dataset.section === button.dataset.sectionTarget);
      if (targetIndex >= 0) goToSlide(targetIndex);
    });
  });

  document.addEventListener("fullscreenchange", () => {
    const isFullscreen = Boolean(document.fullscreenElement);
    fullscreenButton.setAttribute("aria-label", isFullscreen ? "전체 화면 해제" : "전체 화면으로 보기");
    fullscreenButton.title = isFullscreen ? "전체 화면 해제 (Esc)" : "전체 화면";
  });

  document.addEventListener("keydown", (event) => {
    const target = event.target;
    const isTyping = target instanceof HTMLInputElement || target instanceof HTMLTextAreaElement || target?.isContentEditable;
    if (isTyping) return;

    if (slideDialog.open) {
      if (event.key === "Escape") slideDialog.close();
      return;
    }

    const isInteractive = target instanceof HTMLElement
      && Boolean(target.closest("button, a, video, [role='button']"));
    if (isInteractive && (event.code === "Space" || event.key === "Enter")) return;

    if (/^[0-9]$/.test(event.key)) {
      event.preventDefault();
      queueNumberJump(event.key);
      return;
    }

    switch (event.key) {
      case "ArrowRight":
      case "PageDown":
        event.preventDefault();
        moveSlide(1);
        break;
      case "ArrowLeft":
      case "PageUp":
        event.preventDefault();
        moveSlide(-1);
        break;
      case "Home":
        event.preventDefault();
        goToSlide(0, { direction: "prev" });
        break;
      case "End":
        event.preventDefault();
        goToSlide(slides.length - 1, { direction: "next" });
        break;
      default:
        if (event.code === "Space") {
          event.preventDefault();
          moveSlide(1);
        }
    }
  });

  presentation.addEventListener("touchstart", (event) => {
    const touch = event.changedTouches[0];
    touchStartX = touch.clientX;
    touchStartY = touch.clientY;
  }, { passive: true });

  presentation.addEventListener("touchend", (event) => {
    if (touchStartX === null || touchStartY === null) return;
    const touch = event.changedTouches[0];
    const deltaX = touch.clientX - touchStartX;
    const deltaY = touch.clientY - touchStartY;
    touchStartX = null;
    touchStartY = null;

    if (Math.abs(deltaX) < 50 || Math.abs(deltaX) <= Math.abs(deltaY)) return;
    moveSlide(deltaX < 0 ? 1 : -1);
  }, { passive: true });

  window.addEventListener("hashchange", () => {
    goToSlide(getIndexFromHash(), { updateHash: false });
  });

  createSlideList();
  goToSlide(getIndexFromHash());
})();
