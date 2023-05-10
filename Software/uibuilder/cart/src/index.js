// run this function when the document is loaded
window.onload = function () {
    // Start up uibuilder - see the docs for the optional parameters
    uibuilder.start()

    uibuilder.send({
        'topic': 'cart',
        'payload': null,
    })

    // Listen for incoming messages from Node-RED and action
    uibuilder.onChange('msg', (msg) => {
        console.log("🚀 ~ file: index.js:13 ~ uibuilder.onChange ~ msg:", msg)
    })
}
