function checkType(elem, etype) {
    return typeof elem === etype;
}

function log(message) {
    console.log(`[XUI] ${message}`);
}

function createEvent(eventName) {
    return function(data) {
        const eventDetails = {
            ...data,
            timestamp: Date.now(),
            renderCount: data && data.elements ? data.elements.length : 0,
            renderInfo: data && data.elements 
                ? data.elements.map(elem => ({
                    tagName: elem.name || 'Unknown',
                    attributes: elem.attribs ? Object.keys(elem.attribs) : [],
                    childCount: elem.children ? elem.children.length : 0
                }))
                : []
        };

        const event = new CustomEvent(eventName, {
            bubbles: true,
            detail: eventDetails
        });

        return event;
    };
}

// NOTE: uses HTMLDOM because of recursiom
function getEventDispatcher() {
    let eventDispatcher = document.getElementById("_xui_events");
    if (!eventDispatcher) {
        let event_dispatcher = new Element("div", {"id": "_xui_events", "style": "display: none; position: absolute;"});

        eventDispatcher = event_dispatcher.render();
        if (document.body) {
            document.body.insertBefore(eventDispatcher, document.body.firstChild);
        }
    }
    return eventDispatcher;
}

function hasKeyValue(obj, key, value) {
  if (!obj || typeof obj !== "object") return false;
  return obj.hasOwnProperty(key) && obj[key] === value;
}

class Element {
    constructor(name, attribs = {}, ...children) {
        this.name = name;
        this.attribs = {...attribs};
        this.children = [...children];
        this._attr = function (key, value) {
            this.attribs[key] = value;
        };
        this._child = function (child) {
            this.children.push(child);
        };

        this._on = function(eventName, handler) {
            const existingAttr = this.attribs[`on${eventName}`];
            if (existingAttr) {
                const oldHandler = new Function('event', existingAttr);
                this.attribs[`on${eventName}`] = `(${oldHandler})(event); (${handler})(event);`;
            } else {
                this.attribs[`on${eventName}`] = `(${handler})(event)`;
            }
            return this;
        };
        
        if (!hasKeyValue(this.attribs, "id", "_xui_events")) {
            if (document.readyState === "complete" || document.readyState === "interactive") {
                this.dispatchElementCreationEvent();
            } else {
                window.addEventListener("DOMContentLoaded", () => {
                    this.dispatchElementCreationEvent();
                });
            }
        }
    }

    dispatchElementCreationEvent() {
        const eventDispatcher = getEventDispatcher();
        if (eventDispatcher) {
            const elementEvent = createEvent('xui_new_element')({
                name: this.name,
                attribs: this.attribs,
                childCount: this.children.length
            });
            eventDispatcher.dispatchEvent(elementEvent);
        }
    }

    render() {
        let el = document.createElement(this.name);
        
        for (let attr in this.attribs) {
            el.setAttribute(attr, this.attribs[attr]);
        }

        this.children.forEach(child => {
            if (child == null) return;
            
            if (typeof child === "string" || typeof child === "number") {
                el.appendChild(document.createTextNode(String(child)));
            } else if (child instanceof Element) {
                el.appendChild(child.render());
            } else if (child instanceof Node) {
                el.appendChild(child);
            }
        });

        return el;
    }
}

class StyleManager {
    #styles = {};
    #styleElement = null;
    #prefix = 'xui-';
    
    constructor(prefix = 'xui-') {
        this.#prefix = prefix;
    }
    
    init() {
        if (!this.#styleElement) {
            this.#styleElement = document.createElement('style');
            this.#styleElement.id = `${this.#prefix}styles`;
            document.head.appendChild(this.#styleElement);
            log('Style manager initialized');
        }
    }
    
    createRule(name, properties) {
        const ruleName = this.#prefix + name;
        this.#styles[ruleName] = properties;
        this.#updateStyleElement();
        log(`Created style rule: ${ruleName}`);
        return ruleName;
    }
    
    updateRule(name, properties) {
        const ruleName = name.startsWith(this.#prefix) ? name : this.#prefix + name;
        if (this.#styles[ruleName]) {
            this.#styles[ruleName] = {...this.#styles[ruleName], ...properties};
        } else {
            this.#styles[ruleName] = properties;
        }
        this.#updateStyleElement();
        log(`Updated style rule: ${ruleName}`);
        return ruleName;
    }
    
    removeRule(name) {
        const ruleName = name.startsWith(this.#prefix) ? name : this.#prefix + name;
        if (this.#styles[ruleName]) {
            delete this.#styles[ruleName];
            this.#updateStyleElement();
            log(`Removed style rule: ${ruleName}`);
            return true;
        }
        return false;
    }
    
    getRule(name) {
        const ruleName = name.startsWith(this.#prefix) ? name : this.#prefix + name;
        return this.#styles[ruleName] ? ruleName : null;
    }
    
    #createCSS(properties) {
        return Object.entries(properties)
            .map(([key, value]) => {
                const cssKey = key.replace(/([A-Z])/g, '-$1').toLowerCase();
                return `${cssKey}: ${value};`;
            })
            .join(' ');
    }
    
    #updateStyleElement() {
        if (!this.#styleElement) {
            this.init();
        }
        
        let cssText = '';
        for (const [ruleName, properties] of Object.entries(this.#styles)) {
            cssText += `.${ruleName} { ${this.#createCSS(properties)} }\n`;
        }
        
        this.#styleElement.textContent = cssText;
    }
    
    createTheme(theme) {
        const themeRules = {};
        for (const [name, properties] of Object.entries(theme)) {
            const ruleName = this.createRule(name, properties);
            themeRules[name] = ruleName;
        }
        return themeRules;
    }
    
    applyStyles(element, properties) {
        if (element instanceof Element) {
            const cssText = this.#createCSS(properties);
            element.attr('style', cssText);
        } else if (element.attribs) {
            const cssText = this.#createCSS(properties);
            element.attribs.style = cssText;
        }
        return element;
    }
    
    addClass(element, ruleName) {
        const fullRuleName = ruleName.startsWith(this.#prefix) ? ruleName : this.#prefix + ruleName;
        if (element instanceof Element) {
            let classes = element.attribs.class || '';
            if (!classes.includes(fullRuleName)) {
                classes += (classes ? ' ' : '') + fullRuleName;
                element.attr('class', classes);
            }
        } else if (element.attribs) {
            let classes = element.attribs.class || '';
            if (!classes.includes(fullRuleName)) {
                classes += (classes ? ' ' : '') + fullRuleName;
                element.attribs.class = classes;
            }
        }
        return element;
    }
    
    removeClass(element, ruleName) {
        const fullRuleName = ruleName.startsWith(this.#prefix) ? ruleName : this.#prefix + ruleName;
        if (element instanceof Element) {
            let classes = element.attribs.class || '';
            element.attr('class', classes.replace(new RegExp(`\\b${fullRuleName}\\b`, 'g'), '').trim());
        } else if (element.attribs) {
            let classes = element.attribs.class || '';
            element.attribs.class = classes.replace(new RegExp(`\\b${fullRuleName}\\b`, 'g'), '').trim();
        }
        return element;
    }
    
    getAllStyles() {
        return {...this.#styles};
    }
}


class XUIStyle {
    constructor(dom) {
        this.manager = new StyleManager();
        this.dom = dom;
    }
    
    init() {
        this.manager.init();
        return this;
    }
    
    create(name, properties) {
        return this.manager.createRule(name, properties);
    }
    
    update(name, properties) {
        return this.manager.updateRule(name, properties);
    }
    
    remove(name) {
        return this.manager.removeRule(name);
    }
    
    apply(element, name) {
        return this.manager.addClass(element, name);
    }
    
    unapply(element, name) {
        return this.manager.removeClass(element, name);
    }
    
    inline(element, properties) {
        return this.manager.applyStyles(element, properties);
    }
    
    theme(themeObject) {
        return this.manager.createTheme(themeObject);
    }
    
    get(name) {
        return this.manager.getRule(name);
    }
    
    all() {
        return this.manager.getAllStyles();
    }
}

class DOM {
    #elements = [];
    #userOnload = null;
    #DOM_State = {
        "rendered": false,
        "initialized": false
    };
    
    title = "XUI DOM"; // type = string
    root = null;
    options = {
        "remove_elements_on_rerender": true,
        "auto_render": true,
    };
    events = {
        "render": createEvent('xui_render'),
        "element_creation": createEvent('xui_new_element'),
    };
    
    constructor() {
        this.style = new XUIStyle(this);
    }

    init() {
        if (this.#DOM_State["initialized"]) {
            log("DOM already initialized, skipping");
            return;
        }
        
        if (!checkType(this.title, "string")) return;
        log(`initialized with title: ${this.title}`);
        document.title = this.title;
        this.style.init();
        
        this.#DOM_State["initialized"] = true;
    }

    newElement(name, attribs = {}, ...children) {
        let elem = new Element(name, attribs, ...children);
        this.#elements.push(elem);

        return elem;
    }
    
    removeElement(name, attribs = {}) {
        this.#elements = this.#elements.filter(elem => 
            !(elem.name === name && 
             Object.keys(attribs).every(key => elem.attribs[key] === attribs[key]))
        );
    }

    renderAll() {
        if (this.root == null) return;
        
        if (this.options["remove_elements_on_rerender"] === true) {
            this.root.innerHTML = '';
        }
        
        if (this.#DOM_State["rendered"] == true) {
            log(`Rerendering ${this.#elements.length} elements`);
        } else {
            log(`Rendering ${this.#elements.length} elements for the first time`);
        }
        
        const render_event = this.events["render"]({
            elements: [...this.#elements],
        });
        
        let eventDispatcher = getEventDispatcher();

        eventDispatcher.dispatchEvent(render_event);

        this.#elements.forEach(elem => this.root.appendChild(elem.render()));
        this.#DOM_State["rendered"] = true;
    }
        
    clearElements() {
        this.#elements = [];
        if (this.root) {
            this.root.innerHTML = '';
        }
        this.#DOM_State["rendered"] = false;
    }

    setOnload(callback) {
        this.#userOnload = callback;
    }

    getOnload() {
        return this.#userOnload;
    }
    
    executeOnload() {
        const userOnload = this.getOnload();
        if (typeof userOnload === 'function') {
            userOnload();
        }
    }
}

class XE { // XUI Elements 
    constructor(dom) {
        this.dom = dom;
    }

    createElement(tag, attribs, ...children) {
        return this.dom.newElement(tag, attribs, ...children);
    }

    div(attribs, ...children) { return this.createElement("div", attribs, ...children); }
    span(attribs, ...children) { return this.createElement("span", attribs, ...children); }
    p(attribs, ...children) { return this.createElement("p", attribs, ...children); }
    h1(attribs, ...children) { return this.createElement("h1", attribs, ...children); }
    h2(attribs, ...children) { return this.createElement("h2", attribs, ...children); }
    h3(attribs, ...children) { return this.createElement("h3", attribs, ...children); }
    h4(attribs, ...children) { return this.createElement("h4", attribs, ...children); }
    h5(attribs, ...children) { return this.createElement("h5", attribs, ...children); }
    h6(attribs, ...children) { return this.createElement("h6", attribs, ...children); }
    ul(attribs, ...children) { return this.createElement("ul", attribs, ...children); }
    ol(attribs, ...children) { return this.createElement("ol", attribs, ...children); }
    li(attribs, ...children) { return this.createElement("li", attribs, ...children); }
    a(attribs, ...children) { return this.createElement("a", attribs, ...children); }
    img(attribs) { return this.createElement("img", attribs); }
    button(attribs, ...children) { return this.createElement("button", attribs, ...children); }
    input(attribs) { return this.createElement("input", attribs); }
    textarea(attribs, ...children) { return this.createElement("textarea", attribs, ...children); }
    form(attribs, ...children) { return this.createElement("form", attribs, ...children); }
    table(attribs, ...children) { return this.createElement("table", attribs, ...children); }
    tr(attribs, ...children) { return this.createElement("tr", attribs, ...children); }
    td(attribs, ...children) { return this.createElement("td", attribs, ...children); }
    th(attribs, ...children) { return this.createElement("th", attribs, ...children); }
    thead(attribs, ...children) { return this.createElement("thead", attribs, ...children); }
    tbody(attribs, ...children) { return this.createElement("tbody", attribs, ...children); }
    footer(attribs, ...children) { return this.createElement("footer", attribs, ...children); }
    header(attribs, ...children) { return this.createElement("header", attribs, ...children); }
    section(attribs, ...children) { return this.createElement("section", attribs, ...children); }
    article(attribs, ...children) { return this.createElement("article", attribs, ...children); }
    nav(attribs, ...children) { return this.createElement("nav", attribs, ...children); }
    aside(attribs, ...children) { return this.createElement("aside", attribs, ...children); }
    main(attribs, ...children) { return this.createElement("main", attribs, ...children); }
    video(attribs, ...children) { return this.createElement("video", attribs, ...children); }
    audio(attribs, ...children) { return this.createElement("audio", attribs, ...children); }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

// global variables
let gDOM = new DOM(); 
let gXE = new XE(gDOM);
let gEvents;

const gStyle = {
    create: function(name, properties) { return gDOM.style.create(name, properties); },
    apply: function(element, name) { return gDOM.style.apply(element, name); },
    inline: function(element, properties) { return gDOM.style.inline(element, properties); },
    theme: function(themeObject) { return gDOM.style.theme(themeObject); },
    update: function(name, properties) { return gDOM.style.update(name, properties); },
    remove: function(name) { return gDOM.style.remove(name); },
    unapply: function(element, name) { return gDOM.style.unapply(element, name); }
};

let onloadExecuted = false;

window.onload = () => {
    if (onloadExecuted) {
        log("onload already executed, skipping");
        return;
    }
    
    onloadExecuted = true;
    
    gDOM.root = document.body;
   
    let event_dispatcher = new Element("div", {"id": "_xui_events", "style": "display: none; position: absolute;"});

    gEvents = event_dispatcher.render();
    gEvents.on = function(eventName, handler) {
        if (!(gEvents instanceof HTMLElement)) {
            console.warn("[XUI] gEvents is not a valid DOM element");
            return;
        }

        gEvents.addEventListener(eventName, handler);
    };

    document.body.insertBefore(gEvents, document.body.firstChild);
   
    if (gDOM.options.auto_render) {
        eventDispatcher.addEventListener('xui_new_element', () => {
            if (!gDOM.#state.rendered) {
                gDOM.renderAll();
            }
        });
    }

    const rootElement = document.getElementById("root");
    if (rootElement) {
        gDOM.root = rootElement;
    } else {
        console.warn("[XUI] No 'root' element found using body");
    }

    gDOM.init();

    gDOM.executeOnload();
};

