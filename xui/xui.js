function checkType(elem, etype) {
    return typeof elem === etype;
}

function log(message) {
    console.log(`[XUI] ${message}`);
}

function createEvent(eventName) {
    return function(data) {
        const event = new CustomEvent(eventName, {
            bubbles: true,
            detail: data 
        });
        return event;
    };
}

class Element {
    constructor(name, attribs = {}, ...children) {
        this.name = name;
        this.attribs = {...attribs};
        this.children = [...children];
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

    appendChild(child) {
        this.children.push(child);
    }

    appendAttribute(key, value) {
        this.attribs[key] = value;
    }
    
}

class DOM {
    #elements = [];
    #userOnload = null;
    #DOM_State = {
        "rendered": false,
    };
    
    title = "XUI DOM"; // type = string
    root = null;
    options = {
        "remove_elements_on_rerender": true,
    };
    events = {
        "render": createEvent('xui_render'),
    };


    init() {
        if (!checkType(this.title, "string")) return;
        log(`initialized with title: ${this.title}`);
        document.title = this.title;
    }

    newElement(name, attribs = {}, ...children) {
        let elem = new Element(name, attribs, ...children);
        this.#elements.push(elem);
        return elem;
    }

    renderAll() {
        if (this.root == null) return;
        if (this.#DOM_State["rendered"] == true) {
            if (this.options["remove_elements_on_rerender"] == true) {
                this.root.innerHTML = '';
            }
            log(`Rerendering ${this.#elements.length} elements`);
        } else {
            log(`Rendering ${this.#elements.length} elements`);
        }
        const render_event = this.events["render"]({
            elements: [...this.#elements]
        });
        document.getElementById("_xui_events").dispatchEvent(render_event);

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

window.onload = () => {
    gDOM.root = document.body;
    
    const eventsContainer = document.createElement('div');
    eventsContainer.id = '_xui_events';
    eventsContainer.style.display = 'none';
    eventsContainer.style.position = 'absolute';
    document.body.insertBefore(eventsContainer, document.body.firstChild);
    
    gEvents = eventsContainer;

    const rootElement = document.getElementById("root");
    if (rootElement) {
        gDOM.root = rootElement;
    } else {
        console.warn("[XUI] No 'root' element found using body");
    }

    gDOM.init();

    const userOnload = gDOM.getOnload();
    if (typeof userOnload === 'function') {
        userOnload();
    }
};





