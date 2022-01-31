function createCells(xoffset, yoffset, width, height, numCols, numRows)
{
    let cellWidth = width/numCols;
    let cellHeight = height/numRows;

    let layout = [];

    for (let r = 0 ; r < numRows ; r++)
    {
        let y0 = yoffset + r*cellHeight;
        let y1 = y0 + cellHeight;
        for (let c = 0 ; c < numCols ; c++)
        {
            let x0 = xoffset + c*cellWidth;
            let x1 = x0 + cellWidth;

            layout.push([x0, y0, x1, y1]);
        }
    }
    return layout;
}

function generateLayouts()
{
    let layoutsForCells = {};

    let addLayout = (l) => {
        let numCells = l.length;
        if (!(numCells in layoutsForCells))
            layoutsForCells[numCells] = [ l ];
        else
            layoutsForCells[numCells].push(l);
    }

    for (let r = 1 ; r <= 4 ; r++)
    {
        for (let c = 1 ; c <= 4 ; c++)
        {
            // start with r*c cells, check if last row/column can be less
            addLayout(createCells(0,0,1,1,c,r));

            if (r > 1 && c > 1)
            {
                let partHeight = 1-1/r;

                let l0 = createCells(0,0,1,partHeight,c,r-1);
                for (let cpart = 1 ; cpart < c ; cpart++)
                    addLayout([...l0, ...createCells(0, partHeight, 1, 1/r, cpart, 1)]);

                let partWidth = 1-1/c;
                l0 = createCells(0,0,partWidth,1,c-1,r);
                for (let rpart = 1 ; rpart < r ; rpart++)
                    addLayout([...l0, ...createCells(partWidth, 0, 1/c, 1, 1, rpart)]);
            }
        }
    }

    return layoutsForCells;
}

const layoutsForCells = generateLayouts();

let videosAndNames = [];

// Just a very basic implementation for now

function createVideoElement(uuid, displayName)
{
    let vid = document.createElement("video");
    vid.setAttribute("autoplay", "");
    // vid.setAttribute("controls", "");
    vid.setAttribute("muted", "");
    vid.style.width = "100%";
    vid.style.height = "100%";
    vid.style.objectFit = "contain";
    vid.id = uuid;

    videosAndNames.push([vid, displayName]);

    document.body.appendChild(vid);
    fillLayout();

    return vid;
}

function removeVideo(uuid)
{
    let found = false;
    for (let i = 0 ; i < videosAndNames.length ; i++)
    {
        if (videosAndNames[i][0].id == uuid)
        {
            videosAndNames.splice(0,1);
            found = true;
            break;
        }
    }

    if (!found)
        console.warn("Uuid not found in videosAndNames: " + uuid);
    else 
        fillLayout();
}

let currentLayoutIndex = 0;

function fillLayout()
{
    let layout = layoutsForCells[videosAndNames.length][0];
    currentLayoutIndex = 0;

    applyLayout(layout);

    let cells = document.querySelectorAll(".cell");
    for (let i = 0 ; i < cells.length ; i++)
    {
        let cell = cells[i];
        let [ vid, name ] = videosAndNames[i];

        cell.innerHTML = "";
        cell.appendChild(vid);

        let div = document.createElement("div");
        div.style.position = "absolute";
        div.style.bottom = "0px";
        div.innerText = name;

        cell.appendChild(div);
    }
}

function applyLayout(layout)
{
    let parent = document.getElementById("container");
 
    let currentCells = [];
    for (let cell of document.querySelectorAll(".cell"))
        currentCells.push(cell);
   
    // make sure there are enough cells
    if (currentCells.length < layout.length)
    {
        let num = layout.length-currentCells.length;
        for (let i = 0 ; i < num ; i++)
        {
            let d = document.createElement("div");
            d.className = "cell";
            parent.appendChild(d);
            currentCells.push(d);
        }
    }

    // set their coordinates
    for (let i = 0 ; i < layout.length ; i++)
    {
        let [ x0, y0, x1, y1 ] = layout[i];
        let d = currentCells[i];
        d.style.left = `${x0*100}%`;
        d.style.top = `${y0*100}%`;
        d.style.width = `${(x1-x0)*100}%`;
        d.style.height = `${(y1-y0)*100}%`;
    }

    // remove excess cells
    for (let i = layout.length ; i < currentCells.length ; i++)
        parent.removeChild(currentCells[i]);
}

function toggleNextLayout()
{
    let layouts = layoutsForCells[videosAndNames.length];
    currentLayoutIndex = (currentLayoutIndex+1)%layouts.length;

    applyLayout(layouts[currentLayoutIndex]);
}