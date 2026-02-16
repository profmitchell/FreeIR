Use a sleek, modern interface with a "dark glass" aesthetic featuring:
COLOR PALETTE:
* Primary background: Rich black (#000000 to #0A0A0A) with subtle gradient
* Secondary surfaces: Semi-transparent black (rgba(0,0,0,0.4)) with backdrop blur
* Accent elements: Pure white (#FFFFFF) with reduced opacity (40-80%)
* Highlights/glows: Soft white glow (rgba(255,255,255,0.1))
* Text: White primary (#FFFFFF), light gray secondary (#AAAAAA)
GLASS EFFECT:
* Apply backdrop-blur (8-15px) to container elements
* Use background-color: rgba(0,0,0,0.4) to rgba(30,30,30,0.3)
* Add subtle border: 1px solid rgba(255,255,255,0.1)
* Implement subtle inner glow on hover: box-shadow: inset 0 0 15px rgba(255,255,255,0.05)
DEPTH & SHADOWS:
* Outer glow on primary containers: box-shadow: 0 0 15px rgba(255,255,255,0.1)
* Subtle drop shadows between elements: 0 4px 6px rgba(0,0,0,0.1)
* Layer elements with z-index and subtle shadows to create depth
* Use shadow-[0_0_10px_rgba(255,255,255,0.05)] for subtle white glow
BORDERS & CORNERS:
* Rounded corners (border-radius: 1rem or 16px) for containers
* More aggressive rounding (border-radius: 9999px) for buttons and interactive elements
* Thin borders (1px) with low opacity white (rgba(255,255,255,0.1))
* Border opacity increases on hover/focus states
TYPOGRAPHY:
* Clean, modern sans-serif (Inter, SF Pro, or similar)
* Font weights: Light (300) for large headings, Regular (400) for body, Medium (500) for emphasis
* Generous letter-spacing for headings (tracking-wide)
* Text shadows for headings: 0 0 10px rgba(255,255,255,0.2)
INTERACTIVE ELEMENTS:
* Buttons: Semi-transparent (bg-white/10) with hover state (bg-white/20)
* Subtle transition on hover: background-color 0.2s, transform 0.1s
* Micro-animations: Scale 1.02 on hover, scale 0.98 on active
* Focus states with white glow: box-shadow: 0 0 0 2px rgba(255,255,255,0.3)
LAYOUT:
* Generous spacing between elements (gap-4 to gap-8)
* Card-based design with glass effect containers
* Hierarchical structure with primary and secondary containers
* Consistent padding within containers (p-6)
GRADIENTS & BACKGROUNDS:
* Subtle background gradient: from-black to-zinc-900
* Optional: Very subtle noise texture overlay at 2-3% opacity
* Avoid harsh lines; use gradients for transitions between sections
SPECIAL EFFECTS:
* White glow on primary CTAs: shadow-[0_0_15px_rgba(255,255,255,0.1)]
* Subtle parallax on scroll for background elements
* Reduced opacity + blur for disabled states
* Subtle animation for loading states (pulse effect with white glow)
IMPLEMENTATION NOTES:
* Use backdrop-filter: blur() with fallbacks for browser compatibility
* Implement with Tailwind using custom theme extensions for consistent styling
* For non-Tailwind, use CSS variables to maintain consistency
* Test on dark backgrounds to ensure proper contrast and glow effects
## Tailwind CSS Implementation

If you're using Tailwind CSS (as in our app), here's how you would implement this aesthetic:

```javascript
// tailwind.config.js
module.exports = {
  theme: {
    extend: {
      colors: {
        background: "#000000",
        surface: "#0A0A0A",
        // Use rgba in the actual CSS
      },
      boxShadow: {
        'glass-inner': 'inset 0 0 15px rgba(255,255,255,0.05)',
        'glass-outer': '0 0 15px rgba(255,255,255,0.1)',
        'button-glow': '0 0 15px rgba(255,255,255,0.2)',
      },
      borderRadius: {
        'glass': '1rem',
      },
      backdropBlur: {
        'glass': '12px',
      },
    },
  },
}
CSS Implementation Example
Here's a CSS snippet that captures the essence of the dark glass aesthetic:
.glass-container {
  background-color: rgba(10, 10, 10, 0.4);
  backdrop-filter: blur(12px);
  -webkit-backdrop-filter: blur(12px);
  border-radius: 16px;
  border: 1px solid rgba(255, 255, 255, 0.1);
  box-shadow: 0 0 15px rgba(255, 255, 255, 0.1);
  color: white;
  padding: 24px;
}

.glass-button {
  background-color: rgba(255, 255, 255, 0.1);
  border-radius: 9999px;
  border: none;
  color: white;
  padding: 10px 20px;
  font-weight: 500;
  transition: all 0.2s ease;
  box-shadow: 0 0 10px rgba(255, 255, 255, 0.05);
}

.glass-button:hover {
  background-color: rgba(255, 255, 255, 0.2);
  box-shadow: 0 0 15px rgba(255, 255, 255, 0.15);
  transform: scale(1.02);
}

.glass-button:active {
  transform: scale(0.98);
}

body {
  background: linear-gradient(to bottom, #000000, #18181b);
  color: white;
  font-family: 'Inter', sans-serif;
}

h1, h2, h3 {
  letter-spacing: 0.05em;
  font-weight: 300;
}
Key Elements That Make It Work
What makes this aesthetic so effective:
1. Contrast between pure black and white glows - The stark contrast creates a premium, high-tech feel
2. Subtle transparency with backdrop blur - Creates depth without losing readability
3. Consistent rounded corners - Softens the interface and creates a modern feel
4. White glow effects - Adds dimension and draws attention to important
