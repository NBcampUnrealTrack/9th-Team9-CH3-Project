(() => {
  "use strict";

  const CANVAS_WIDTH = 1600;
  const CANVAS_HEIGHT = 900;
  const HASH_PREFIX = "#slide-";
  const SWIPE_THRESHOLD = 55;

  const deckViewport = document.getElementById("deckViewport");
  const deckScaler = document.getElementById("deckScaler");
  const deck = document.getElementById("deck");
  const slides = Array.from(deck.querySelectorAll(".slide"));
  const prevButton = document.getElementById("prevButton");
  const nextButton = document.getElementById("nextButton");
  const currentSlideNumber = document.getElementById("currentSlideNumber");
  const totalSlideNumber = document.getElementById("totalSlideNumber");
  const progressBar = document.getElementById("progressBar");
  const sectionNav = document.getElementById("sectionNav");
  const fullscreenButton = document.getElementById("fullscreenButton");
  const openSlideList = document.getElementById("openSlideList");
  const closeSlideList = document.getElementById("closeSlideList");
  const slideDialog = document.getElementById("slideDialog");
  const slideList = document.getElementById("slideList");
  const slideAnnouncement = document.getElementById("slideAnnouncement");
  const coverMedia = document.querySelector("[data-cover-media]");

  if (!slides.length) {
    return;
  }

  let currentIndex = 0;
  let touchStartX = 0;
  let touchStartY = 0;
  let numberBuffer = "";
  let numberTimer = 0;

  const formatNumber = (value) => String(value).padStart(2, "0");

  const initializeCoverMedia = () => {
    if (!coverMedia) return;

    const image = coverMedia.querySelector("img");
    if (!image) return;

    const updateCoverState = () => {
      const hasImage = image.naturalWidth > 0 && image.naturalHeight > 0;
      coverMedia.classList.toggle("has-image", hasImage);
    };

    image.addEventListener("load", updateCoverState);
    image.addEventListener("error", updateCoverState);
    if (image.complete) updateCoverState();
  };

  const parseHashIndex = () => {
    const match = window.location.hash.match(/^#slide-(\d+)$/);
    if (!match) return 0;
    return Math.min(Math.max(Number(match[1]) - 1, 0), slides.length - 1);
  };

  const slideTitle = (slide) => {
    const heading = slide.querySelector("h1, h2");
    return heading ? heading.textContent.trim() : slide.id;
  };

  const buildSectionNavigation = () => {
    const firstIndexBySection = new Map();
    slides.forEach((slide, index) => {
      const section = slide.dataset.section || "SECTION";
      if (!firstIndexBySection.has(section)) firstIndexBySection.set(section, index);
    });

    firstIndexBySection.forEach((index, section) => {
      const button = document.createElement("button");
      button.type = "button";
      button.textContent = section;
      button.dataset.section = section;
      button.dataset.slideIndex = String(index);
      button.setAttribute("aria-label", `${section} 섹션으로 이동`);
      button.addEventListener("click", () => goToSlide(index));
      sectionNav.appendChild(button);
    });
  };

  const buildSlideList = () => {
    slides.forEach((slide, index) => {
      const button = document.createElement("button");
      button.type = "button";
      button.dataset.slideIndex = String(index);
      button.innerHTML = `<b>${formatNumber(index + 1)}</b><span>${slideTitle(slide)}</span>`;
      button.addEventListener("click", () => {
        goToSlide(index);
        slideDialog.close();
      });
      slideList.appendChild(button);
    });
  };

  const updateSectionNavigation = () => {
    const currentSection = slides[currentIndex].dataset.section;
    sectionNav.querySelectorAll("button").forEach((button) => {
      const isCurrent = button.dataset.section === currentSection;
      button.classList.toggle("is-current", isCurrent);
      button.setAttribute("aria-current", isCurrent ? "true" : "false");
    });
  };

  const updateSlideList = () => {
    slideList.querySelectorAll("button").forEach((button, index) => {
      const isCurrent = index === currentIndex;
      button.classList.toggle("is-current", isCurrent);
      button.setAttribute("aria-current", isCurrent ? "page" : "false");
    });
  };

  const updateHash = (replace = false) => {
    const nextHash = `${HASH_PREFIX}${currentIndex + 1}`;
    if (window.location.hash === nextHash) return;
    if (replace) {
      window.history.replaceState(null, "", nextHash);
    } else {
      window.history.pushState(null, "", nextHash);
    }
  };

  const renderSlide = (previousIndex, replaceHash = false) => {
    slides.forEach((slide, index) => {
      const isActive = index === currentIndex;
      slide.classList.toggle("is-active", isActive);
      slide.classList.toggle("is-leaving-left", index === previousIndex && currentIndex > previousIndex);
      slide.setAttribute("aria-hidden", String(!isActive));
      if ("inert" in slide) slide.inert = !isActive;
    });

    currentSlideNumber.textContent = formatNumber(currentIndex + 1);
    totalSlideNumber.textContent = formatNumber(slides.length);
    progressBar.style.width = `${((currentIndex + 1) / slides.length) * 100}%`;
    prevButton.disabled = currentIndex === 0;
    nextButton.disabled = currentIndex === slides.length - 1;
    updateSectionNavigation();
    updateSlideList();
    updateHash(replaceHash);
    document.title = `${formatNumber(currentIndex + 1)} · ${slideTitle(slides[currentIndex])} — Parcel Knight`;
    slideAnnouncement.textContent = `${currentIndex + 1}번 슬라이드, ${slideTitle(slides[currentIndex])}`;
  };

  function goToSlide(index, options = {}) {
    const nextIndex = Math.min(Math.max(index, 0), slides.length - 1);
    const previousIndex = currentIndex;
    currentIndex = nextIndex;
    renderSlide(previousIndex, Boolean(options.replaceHash));
  }

  const nextSlide = () => goToSlide(currentIndex + 1);
  const previousSlide = () => goToSlide(currentIndex - 1);

  const resizeDeck = () => {
    const availableWidth = deckViewport.clientWidth;
    const availableHeight = deckViewport.clientHeight;
    const scale = Math.min(availableWidth / CANVAS_WIDTH, availableHeight / CANVAS_HEIGHT);
    deckScaler.style.width = `${CANVAS_WIDTH * scale}px`;
    deckScaler.style.height = `${CANVAS_HEIGHT * scale}px`;
    deck.style.transformOrigin = "top left";
    deck.style.transform = `scale(${scale})`;
  };

  const isEditableTarget = (target) => {
    if (!(target instanceof Element)) return false;
    return Boolean(target.closest("input, textarea, select, [contenteditable='true']"));
  };

  const commitNumberBuffer = () => {
    window.clearTimeout(numberTimer);
    if (!numberBuffer) return;
    const requested = Number(numberBuffer);
    numberBuffer = "";
    if (requested >= 1 && requested <= slides.length) goToSlide(requested - 1);
  };

  const handleNumberKey = (key) => {
    window.clearTimeout(numberTimer);
    numberBuffer += key;
    const requested = Number(numberBuffer);
    if (requested > slides.length) {
      numberBuffer = key;
    }
    numberTimer = window.setTimeout(commitNumberBuffer, 650);
  };

  const toggleFullscreen = async () => {
    try {
      if (!document.fullscreenElement) {
        await document.documentElement.requestFullscreen();
      } else {
        await document.exitFullscreen();
      }
    } catch (error) {
      console.warn("전체 화면 전환을 완료하지 못했습니다.", error);
    }
  };

  const updateFullscreenButton = () => {
    const active = Boolean(document.fullscreenElement);
    fullscreenButton.setAttribute("aria-label", active ? "전체 화면 종료" : "전체 화면 시작");
    fullscreenButton.title = active ? "전체 화면 종료" : "전체 화면";
  };

  prevButton.addEventListener("click", previousSlide);
  nextButton.addEventListener("click", nextSlide);
  fullscreenButton.addEventListener("click", toggleFullscreen);
  document.addEventListener("fullscreenchange", updateFullscreenButton);

  openSlideList.addEventListener("click", () => {
    slideDialog.showModal();
    const currentButton = slideList.querySelector("button.is-current");
    if (currentButton) currentButton.focus();
  });
  closeSlideList.addEventListener("click", () => slideDialog.close());
  slideDialog.addEventListener("click", (event) => {
    if (event.target === slideDialog) slideDialog.close();
  });

  window.addEventListener("keydown", (event) => {
    if (isEditableTarget(event.target)) return;

    if (/^[0-9]$/.test(event.key)) {
      handleNumberKey(event.key);
      return;
    }

    const key = event.key.toLowerCase();
    if (key === "f") {
      event.preventDefault();
      toggleFullscreen();
      return;
    }

    switch (event.key) {
      case "ArrowRight":
      case "PageDown":
        event.preventDefault();
        nextSlide();
        break;
      case "ArrowLeft":
      case "PageUp":
        event.preventDefault();
        previousSlide();
        break;
      case " ":
        if (!slideDialog.open) {
          event.preventDefault();
          nextSlide();
        }
        break;
      case "Home":
        event.preventDefault();
        goToSlide(0);
        break;
      case "End":
        event.preventDefault();
        goToSlide(slides.length - 1);
        break;
      default:
        break;
    }
  });

  deckViewport.addEventListener("touchstart", (event) => {
    const touch = event.changedTouches[0];
    touchStartX = touch.clientX;
    touchStartY = touch.clientY;
  }, { passive: true });

  deckViewport.addEventListener("touchend", (event) => {
    const touch = event.changedTouches[0];
    const deltaX = touch.clientX - touchStartX;
    const deltaY = touch.clientY - touchStartY;
    if (Math.abs(deltaX) < SWIPE_THRESHOLD || Math.abs(deltaX) <= Math.abs(deltaY)) return;
    if (deltaX < 0) nextSlide();
    else previousSlide();
  }, { passive: true });

  window.addEventListener("hashchange", () => {
    const hashIndex = parseHashIndex();
    if (hashIndex !== currentIndex) goToSlide(hashIndex, { replaceHash: true });
  });
  window.addEventListener("resize", resizeDeck);

  buildSectionNavigation();
  buildSlideList();
  initializeCoverMedia();
  currentIndex = parseHashIndex();
  resizeDeck();
  renderSlide(currentIndex, true);
})();
