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

function debounce(func, wait) {
    let timeout;
    return function(...args) {
        clearTimeout(timeout);
        timeout = setTimeout(() => func.apply(this, args), wait);
    };
}

function throttle(func, limit) {
    let inThrottle;
    return function(...args) {
        if (!inThrottle) {
            func.apply(this, args);
            inThrottle = true;
            setTimeout(() => inThrottle = false, limit);
        }
    };
}

class EventEmitter {
    constructor() {
        this.events = {};
    }
    
    on(event, listener) {
        if (!this.events[event]) {
            this.events[event] = [];
        }
        this.events[event].push(listener);
        return this;
    }
    
    off(event, listener) {
        if (!this.events[event]) return this;
        this.events[event] = this.events[event].filter(l => l !== listener);
        return this;
    }
    
    emit(event, ...args) {
        if (!this.events[event]) return false;
        this.events[event].forEach(listener => listener.apply(this, args));
        return true;
    }
    
    once(event, listener) {
        const onceWrapper = (...args) => {
            listener.apply(this, args);
            this.off(event, onceWrapper);
        };
        return this.on(event, onceWrapper);
    }
}

class Element {
    constructor(name, attribs = {}, ...children) {
        this.name = name;
        this.attribs = {...attribs};
        this.children = [...children];
        this._events = {};
        
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

    attr(key, value) {
        this.attribs[key] = value;
        return this;
    }

    attrs(attributes) {
        for (const [key, value] of Object.entries(attributes)) {
            this.attribs[key] = value;
        }
        return this;
    }

    child(child) {
        this.children.push(child);
        return this;
    }

    children(childrenArray) {
        this.children.push(...childrenArray);
        return this;
    }

    clearChildren() {
        this.children = [];
        return this;
    }

    removeChild(child) {
        const index = this.children.indexOf(child);
        if (index !== -1) {
            this.children.splice(index, 1);
        }
        return this;
    }

    on(eventName, handler) {
        const existingAttr = this.attribs[`on${eventName}`];
        if (existingAttr) {
            const oldHandler = new Function('event', existingAttr);
            this.attribs[`on${eventName}`] = `(${oldHandler})(event); (${handler})(event);`;
        } else {
            this.attribs[`on${eventName}`] = `(${handler})(event)`;
        }
        this._events[eventName] = handler;
        return this;
    }

    off(eventName) {
        delete this.attribs[`on${eventName}`];
        delete this._events[eventName];
        return this;
    }

    addClass(className) {
        const currentClasses = this.attribs.class ? this.attribs.class.split(' ') : [];
        if (!currentClasses.includes(className)) {
            currentClasses.push(className);
            this.attribs.class = currentClasses.join(' ');
        }
        return this;
    }

    removeClass(className) {
        if (this.attribs.class) {
            const currentClasses = this.attribs.class.split(' ');
            const index = currentClasses.indexOf(className);
            if (index !== -1) {
                currentClasses.splice(index, 1);
                this.attribs.class = currentClasses.join(' ');
            }
        }
        return this;
    }

    toggleClass(className) {
        const currentClasses = this.attribs.class ? this.attribs.class.split(' ') : [];
        const index = currentClasses.indexOf(className);
        if (index !== -1) {
            currentClasses.splice(index, 1);
        } else {
            currentClasses.push(className);
        }
        this.attribs.class = currentClasses.join(' ');
        return this;
    }

    hasClass(className) {
        const currentClasses = this.attribs.class ? this.attribs.class.split(' ') : [];
        return currentClasses.includes(className);
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

    html(content) {
        this.clearChildren();
        if (typeof content === 'string') {
            this.child(content);
        }
        return this;
    }

    text(content) {
        this.clearChildren();
        this.child(String(content));
        return this;
    }

    css(property, value) {
        const currentStyle = this.attribs.style || '';
        const propertyName = property.replace(/([A-Z])/g, '-$1').toLowerCase();
        
        const styleObj = {};
        currentStyle.split(';').forEach(style => {
            const [prop, val] = style.split(':');
            if (prop && val) {
                styleObj[prop.trim()] = val.trim();
            }
        });
        
        styleObj[propertyName] = value;
        
        const newStyle = Object.entries(styleObj)
            .map(([prop, val]) => `${prop}: ${val}`)
            .join('; ');
            
        this.attribs.style = newStyle;
        return this;
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

    clone() {
        const newElement = new Element(this.name, {...this.attribs});
        newElement.children = this.children.map(child => {
            if (child instanceof Element) {
                return child.clone();
            }
            return child;
        });
        return newElement;
    }

    find(selector) {
        if (selector.startsWith('#')) {
            const id = selector.substring(1);
            if (this.attribs.id === id) {
                return this;
            }
        } else if (selector.startsWith('.')) {
            const className = selector.substring(1);
            if (this.hasClass(className)) {
                return this;
            }
        } else if (this.name === selector) {
            return this;
        }

        for (const child of this.children) {
            if (child instanceof Element) {
                const found = child.find(selector);
                if (found) {
                    return found;
                }
            }
        }

        return null;
    }

    findAll(selector) {
        const results = [];

        if (selector.startsWith('#')) {
            const id = selector.substring(1);
            if (this.attribs.id === id) {
                results.push(this);
            }
        } else if (selector.startsWith('.')) {
            const className = selector.substring(1);
            if (this.hasClass(className)) {
                results.push(this);
            }
        } else if (this.name === selector) {
            results.push(this);
        }

        for (const child of this.children) {
            if (child instanceof Element) {
                const found = child.findAll(selector);
                results.push(...found);
            }
        }

        return results;
    }
}

class StyleManager {
    #styles = {};
    #styleElement = null;
    #prefix = 'xui-';
    #animations = {};
    
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
        
        for (const [animName, props] of Object.entries(this.#animations)) {
            cssText += `@keyframes ${animName} {\n`;
            for (const [step, stepProps] of Object.entries(props)) {
                cssText += `  ${step} { ${this.#createCSS(stepProps)} }\n`;
            }
            cssText += `}\n`;
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
    
    createAnimation(name, keyframes) {
        const animName = `${this.#prefix}anim-${name}`;
        this.#animations[animName] = keyframes;
        this.#updateStyleElement();
        log(`Created animation: ${animName}`);
        return animName;
    }
    
    applyAnimation(element, animName, duration = '1s', timing = 'ease', iterations = '1', direction = 'normal', fillMode = 'none') {
        const fullAnimName = animName.startsWith(`${this.#prefix}anim-`) ? animName : `${this.#prefix}anim-${animName}`;
        
        const animationProperty = `${fullAnimName} ${duration} ${timing} ${iterations} ${direction} ${fillMode}`;
        
        if (element instanceof Element) {
            element.css('animation', animationProperty);
        } else if (element.attribs) {
            const currentStyle = element.attribs.style || '';
            element.attribs.style = `${currentStyle}; animation: ${animationProperty}`;
        }
        
        return element;
    }
    
    createMediaQuery(query, rules) {
        if (!this.#styleElement) {
            this.init();
        }
        
        let cssText = this.#styleElement.textContent || '';
        cssText += `\n@media ${query} {\n`;
        
        for (const [selector, properties] of Object.entries(rules)) {
            cssText += `  ${selector} { ${this.#createCSS(properties)} }\n`;
        }
        
        cssText += '}\n';
        
        this.#styleElement.textContent = cssText;
        log(`Created media query: ${query}`);
    }
    
    getAllStyles() {
        return {...this.#styles};
    }
    
    getAllAnimations() {
        return {...this.#animations};
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
    
    createAnimation(name, keyframes) {
        return this.manager.createAnimation(name, keyframes);
    }
    
    applyAnimation(element, animName, duration, timing, iterations, direction, fillMode) {
        return this.manager.applyAnimation(element, animName, duration, timing, iterations, direction, fillMode);
    }
    
    mediaQuery(query, rules) {
        return this.manager.createMediaQuery(query, rules);
    }
}

class StateManager extends EventEmitter {
    constructor(initialState = {}) {
        super();
        this.state = { ...initialState };
        this.prevState = { ...initialState };
    }
    
    get(key) {
        return key ? this.state[key] : { ...this.state };
    }
    
    set(updates) {
        this.prevState = { ...this.state };
        this.state = { ...this.state, ...updates };
        
        for (const [key, value] of Object.entries(updates)) {
            if (this.prevState[key] !== value) {
                this.emit(`change:${key}`, value, this.prevState[key]);
            }
        }
        
        this.emit('change', this.state, this.prevState);
        return this;
    }
    
    reset() {
        this.prevState = { ...this.state };
        this.state = {};
        this.emit('reset', {}, this.prevState);
        return this;
    }
}

class Router {
    constructor(dom) {
        this.dom = dom;
        this.routes = {};
        this.currentRoute = null;
        this.params = {};
        this.query = {};
        
        window.addEventListener('popstate', this.handlePopState.bind(this));
        
        if (document.readyState === 'complete') {
            this.handleInitialRoute();
        } else {
            window.addEventListener('load', this.handleInitialRoute.bind(this));
        }
    }
    
    addRoute(path, handler) {
        this.routes[path] = handler;
        return this;
    }
    
    navigate(path, params = {}) {
        window.history.pushState({}, '', path);
        this.handleRoute(path, params);
        return this;
    }
    
    handlePopState() {
        this.handleRoute(window.location.pathname);
    }
    
    handleInitialRoute() {
        this.handleRoute(window.location.pathname);
    }
    
    handleRoute(path, extraParams = {}) {
        this.currentRoute = path;
        
        this.query = {};
        const queryString = window.location.search.substring(1);
        if (queryString) {
            queryString.split('&').forEach(pair => {
                const [key, value] = pair.split('=');
                this.query[decodeURIComponent(key)] = decodeURIComponent(value || '');
            });
        }
        
        this.params = { ...extraParams };
        
        let handler = this.routes[path];
        
        if (!handler) {
            const dynamicRoutes = Object.keys(this.routes).filter(route => route.includes(':'));
            
            for (const route of dynamicRoutes) {
                const routeParts = route.split('/');
                const pathParts = path.split('/');
                
                if (routeParts.length === pathParts.length) {
                    let match = true;
                    const params = {};
                    
                    for (let i = 0; i < routeParts.length; i++) {
                        if (routeParts[i].startsWith(':')) {
                            const paramName = routeParts[i].substring(1);
                            params[paramName] = pathParts[i];
                        } else if (routeParts[i] !== pathParts[i]) {
                            match = false;
                            break;
                        }
                    }
                    
                    if (match) {
                        handler = this.routes[route];
                        this.params = { ...this.params, ...params };
                        break;
                    }
                }
            }
        }
        
        if (handler) {
            this.dom.clearElements();
            handler(this.params, this.query);
            this.dom.renderAll();
        } else if (this.routes['*']) {
            this.dom.clearElements();
            this.routes['*'](this.params, this.query);
            this.dom.renderAll();
        } else {
            log(`No route handler found for ${path}`);
        }
    }
    
    getCurrentRoute() {
        return this.currentRoute;
    }
    
    getParams() {
        return { ...this.params };
    }
    
    getQuery() {
        return { ...this.query };
    }
}

class FormValidator {
    constructor() {
        this.rules = {};
        this.errorMessages = {};
    }
    
    addRule(field, rule, message) {
        if (!this.rules[field]) {
            this.rules[field] = [];
            this.errorMessages[field] = [];
        }
        
        this.rules[field].push(rule);
        this.errorMessages[field].push(message);
        
        return this;
    }
    
    validate(formData) {
        const errors = {};
        
        for (const field in this.rules) {
            if (formData.hasOwnProperty(field)) {
                const value = formData[field];
                
                for (let i = 0; i < this.rules[field].length; i++) {
                    const rule = this.rules[field][i];
                    const message = this.errorMessages[field][i];
                    
                    if (!rule(value, formData)) {
                        if (!errors[field]) {
                            errors[field] = [];
                        }
                        errors[field].push(message);
                    }
                }
            }
        }
        
        return {
            isValid: Object.keys(errors).length === 0,
            errors
        };
    }
}

class HTTPClient {
    constructor(baseURL = '') {
        this.baseURL = baseURL;
        this.headers = {
            'Content-Type': 'application/json'
        };
    }
    
    setBaseURL(url) {
        this.baseURL = url;
        return this;
    }
    
    setHeader(key, value) {
        this.headers[key] = value;
        return this;
    }
    
    async request(method, endpoint, data = null, options = {}) {
        const url = `${this.baseURL}${endpoint}`;
        
        const requestOptions = {
            method,
            headers: { ...this.headers, ...options.headers },
            ...options
        };
        
        if (data) {
            requestOptions.body = JSON.stringify(data);
        }
        
        try {
            const response = await fetch(url, requestOptions);
            const contentType = response.headers.get('content-type');
            
            let result;
            if (contentType && contentType.includes('application/json')) {
                result = await response.json();
            } else {
                result = await response.text();
            }
            
            if (!response.ok) {
                throw {
                    status: response.status,
                    statusText: response.statusText,
                    data: result
                };
            }
            
            return result;
        } catch (error) {
            throw error;
        }
    }
    
    get(endpoint, options = {}) {
        return this.request('GET', endpoint, null, options);
    }
    
    post(endpoint, data, options = {}) {
        return this.request('POST', endpoint, data, options);
    }
    
    put(endpoint, data, options = {}) {
        return this.request('PUT', endpoint, data, options);
    }
    
    patch(endpoint, data, options = {}) {
        return this.request('PATCH', endpoint, data, options);
    }
    
    delete(endpoint, options = {}) {
        return this.request('DELETE', endpoint, null, options);
    }
}

class LocalStorage {
    constructor(prefix = 'xui_') {
        this.prefix = prefix;
    }
    
    set(key, value) {
        try {
            const serializedValue = JSON.stringify(value);
            localStorage.setItem(`${this.prefix}${key}`, serializedValue);
            return true;
        } catch (error) {
            log(`Error storing item in localStorage: ${error.message}`);
            return false;
        }
    }
    
    get(key, defaultValue = null) {
        try {
            const serializedValue = localStorage.getItem(`${this.prefix}${key}`);
            return serializedValue === null ? defaultValue : JSON.parse(serializedValue);
        } catch (error) {
            log(`Error retrieving item from localStorage: ${error.message}`);
            return defaultValue;
        }
    }
    
    remove(key) {
        try {
            localStorage.removeItem(`${this.prefix}${key}`);
            return true;
        } catch (error) {
            log(`Error removing item from localStorage: ${error.message}`);
            return false;
        }
    }
    
    clear() {
        try {
            Object.keys(localStorage).forEach(key => {
                if (key.startsWith(this.prefix)) {
                    localStorage.removeItem(key);
                }
            });
            return true;
        } catch (error) {
            log(`Error clearing localStorage: ${error.message}`);
            return false;
        }
    }
    
    getAllKeys() {
        try {
            return Object.keys(localStorage)
                .filter(key => key.startsWith(this.prefix))
                .map(key => key.substring(this.prefix.length));
        } catch (error) {
            log(`Error getting localStorage keys: ${error.message}`);
            return [];
        }
    }
}

class DOM extends EventEmitter {
    #elements = [];
    #userOnload = null;
    #state = {
        "rendered": false,
        "initialized": false
    };
    
    title = "XUI DOM";
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
        super();
        this.style = new XUIStyle(this);
        this.state = new StateManager();
        this.storage = new LocalStorage();
        this.http = new HTTPClient();
        this.validator = new FormValidator();
    }

    init() {
        if (this.#state.initialized) {
            log("DOM already initialized, skipping");
            return this;
        }
        
        if (!checkType(this.title, "string")) return this;
        log(`initialized with title: ${this.title}`);
        document.title = this.title;
        this.style.init();
        
        this.router = new Router(this);
        
        this.#state.initialized = true;
        return this;
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
        return this;
    }
    
    getElementById(id) {
        return this.#elements.find(elem => elem.attribs.id === id) || null;
    }
    
    getElementsByTagName(tagName) {
        return this.#elements.filter(elem => elem.name === tagName);
    }
    
    getElementsByClassName(className) {
        return this.#elements.filter(elem => {
            const classes = elem.attribs.class ? elem.attribs.class.split(' ') : [];
            return classes.includes(className);
        });
    }
    
    querySelector(selector) {
        if (selector.startsWith('#')) {
            return this.getElementById(selector.substring(1));
        } else if (selector.startsWith('.')) {
            const elements = this.getElementsByClassName(selector.substring(1));
            return elements.length > 0 ? elements[0] : null;
        } else {
            const elements = this.getElementsByTagName(selector);
            return elements.length > 0 ? elements[0] : null;
        }
    }
       querySelectorAll(selector) {
        if (selector.startsWith('#')) {
            const element = this.getElementById(selector.substring(1));
            return element ? [element] : [];
        } else if (selector.startsWith('.')) {
            return this.getElementsByClassName(selector.substring(1));
        } else {
            return this.getElementsByTagName(selector);
        }
    }

    renderAll() {
        if (this.root == null) return this;
        
        if (this.options.remove_elements_on_rerender === true) {
            this.root.innerHTML = '';
        }
        
        if (this.#state.rendered) {
            log(`Rerendering ${this.#elements.length} elements`);
        } else {
            log(`Rendering ${this.#elements.length} elements for the first time`);
        }
        
        const render_event = this.events.render({
            elements: [...this.#elements],
        });
        
        let eventDispatcher = getEventDispatcher();
        eventDispatcher.dispatchEvent(render_event);

        this.#elements.forEach(elem => this.root.appendChild(elem.render()));
        this.#state.rendered = true;
        return this;
    }
        
    clearElements() {
        this.#elements = [];
        if (this.root) {
            this.root.innerHTML = '';
        }
        this.#state.rendered = false;
        return this;
    }

    setOnload(callback) {
        this.#userOnload = callback;
        return this;
    }

    getOnload() {
        return this.#userOnload;
    }
    
    executeOnload() {
        const userOnload = this.getOnload();
        if (typeof userOnload === 'function') {
            userOnload();
        }
        return this;
    }

    getStateParameter(name) {
        if (name in this.#state) {
            return this.#state[name];
        } 
        return "";
    }
}

class XE {
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

const gDOM = new DOM(); 
const gXE = new XE(gDOM);
let gEvents;
const gRouter = gDOM.router;
const gState = gDOM.state;
const gStorage = gDOM.storage;
const gHttp = gDOM.http;
const gValidator = gDOM.validator;

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
    if (gEvents instanceof HTMLElement) {
        document.body.insertBefore(gEvents, document.body.firstChild);
    }
   
    if (gDOM.options.auto_render) {
        const eventDispatcher = getEventDispatcher();
        eventDispatcher.addEventListener('xui_new_element', () => {
            if (!gDOM.getStateParameter("rendered")) {
                gDOM.renderAll();
            }
        });
    }

    const rootElement = document.getElementById("root");
    if (rootElement) {
        gDOM.root = rootElement;
    } else {
        log("No 'root' element found using body");
    }

    gDOM.init();
    gDOM.executeOnload();
};

//export { gDOM, gXE, gStyle, Element, Router, StateManager, HTTPClient, LocalStorage, FormValidator, EventEmitter };


